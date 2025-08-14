#include "DynamicBandpassFilter.h"
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <mutex>
#include <thread>
#include <cmath>
#include <algorithm>
#include <QDebug>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Protocol default configurations - Updated with USB/LSB support
struct ProtocolDefaults {
    float passband_width;
    float transition_width;
    float stopband_atten;
    float carrier_offset;      // For SSB modes
    bool sharp_cutoff;         // For SSB modes
};

static const ProtocolDefaults PROTOCOL_DEFAULTS[] = {
    {200000.0f, 0.1f, 60.0f, 0.0f, false},      // WFM
    {12500.0f, 0.15f, 50.0f, 0.0f, false},      // NBFM
    {8000.0f, 0.2f, 40.0f, 0.0f, false},        // AM
    {3000.0f, 0.05f, 70.0f, 1500.0f, true},     // USB - sharp cutoff, 1.5kHz carrier offset
    {3000.0f, 0.05f, 70.0f, -1500.0f, true}     // LSB - sharp cutoff, -1.5kHz carrier offset
};

static const char* protocol_names[] = {"WFM", "NBFM", "AM", "USB", "LSB"};

DynamicBandpassFilter::DynamicBandpassFilter() 
    : initialized_(false)
    , enabled_(false)
    , parameters_changed_(true)
    , processing_active_(false)
    , fft_size_(0)
    , overlap_size_(0)
    , frequency_resolution_(0.0f)
    , forward_plan_(nullptr)
    , inverse_plan_(nullptr)
    , fft_input_(nullptr)
    , fft_output_(nullptr)
    , passband_low_hz_(0.0f)
    , passband_high_hz_(0.0f)
    , current_center_freq_(0.0f)
    , ssb_carrier_offset_(0.0f)
    , ssb_sharp_cutoff_(false)
    , energy_history_idx_(0)
    , adaptive_alpha_(0.05f)
    , total_samples_processed_(0) 
{
    // Initialize default configuration for WFM
    config_.protocol = WFM;
    config_.shape = BLACKMAN;
    config_.stopband_attenuation = 75.0f;
    config_.center_frequency = 0.0f;
    config_.bandwidth = 200000.0f;
    config_.sample_rate = 2048000.0;
    config_.ssb_carrier_offset = 0.0f;
    config_.ssb_sharp_cutoff = false;
    
    // Initialize energy history for adaptive centering
    energy_history_.resize(32, 0.0f);
    
    // Initialize statistics
    current_stats_ = {};
    current_stats_.is_enabled = false;
    current_stats_.ssb_mode_active = false;
    
    qDebug() << "DynamicBandpassFilter: Created with default WFM configuration";
}

DynamicBandpassFilter::~DynamicBandpassFilter() {
    // Wait for any active processing to complete
    while (processing_active_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    cleanup();
}

bool DynamicBandpassFilter::initialize(int sample_rate, int fft_size) {
    if (sample_rate <= 0 || fft_size < 256 || (fft_size & (fft_size - 1)) != 0) {
        qDebug() << "DynamicBandpassFilter: Invalid parameters - sample_rate:" << sample_rate << "fft_size:" << fft_size;
        return false;
    }
    
    // Wait for any active processing to complete
    while (processing_active_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Cleanup any existing resources
    cleanup();
    
    std::lock_guard<std::mutex> state_lock(state_mutex_);
    std::lock_guard<std::mutex> config_lock(config_mutex_);
    std::lock_guard<std::mutex> fft_lock(fft_mutex_);
    
    config_.sample_rate = sample_rate;
    fft_size_.store(fft_size);
    frequency_resolution_.store(static_cast<float>(sample_rate) / fft_size);
    
    try {
        // Allocate FFT buffers using fftwf_malloc for better alignment
        fft_input_ = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fft_size);
        fft_output_ = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fft_size);
        
        if (!fft_input_ || !fft_output_) {
            throw std::runtime_error("Failed to allocate FFT buffers");
        }
        
        // Initialize buffers to zero
        memset(fft_input_, 0, sizeof(fftwf_complex) * fft_size);
        memset(fft_output_, 0, sizeof(fftwf_complex) * fft_size);
        
        // Create FFT plans with FFTW_ESTIMATE (thread-safe)
        forward_plan_ = fftwf_plan_dft_1d(fft_size, fft_input_, fft_output_, FFTW_FORWARD, FFTW_ESTIMATE);
        inverse_plan_ = fftwf_plan_dft_1d(fft_size, fft_output_, fft_input_, FFTW_BACKWARD, FFTW_ESTIMATE);
        
        if (!forward_plan_ || !inverse_plan_) {
            throw std::runtime_error("Failed to create FFT plans");
        }
        
        // Initialize filter kernel - start with all-pass
        {
            std::lock_guard<std::mutex> filter_lock(filter_mutex_);
            filter_kernel_.resize(fft_size, std::complex<float>(1.0f, 0.0f));
        }
        
        // Set initial passband
        const auto& defaults = PROTOCOL_DEFAULTS[static_cast<int>(config_.protocol)];
        float half_bandwidth = defaults.passband_width / 2.0f;
        passband_low_hz_.store(-half_bandwidth);
        passband_high_hz_.store(half_bandwidth);
        current_center_freq_.store(config_.center_frequency);
        ssb_carrier_offset_.store(defaults.carrier_offset);
        ssb_sharp_cutoff_.store(defaults.sharp_cutoff);
        
        // Design initial filter
        designFilter();
        
        // Initialize statistics
        {
            std::lock_guard<std::mutex> stats_lock(stats_mutex_);
            current_stats_ = {};
            current_stats_.is_enabled = enabled_.load();
            current_stats_.ssb_mode_active = (config_.protocol == USB || config_.protocol == LSB);
            current_stats_.ssb_carrier_offset_hz = defaults.carrier_offset;
        }
        
        initialized_.store(true);
        parameters_changed_.store(false);
        
        qDebug() << "DynamicBandpassFilter: Initialized successfully";
        qDebug() << "  Sample rate:" << config_.sample_rate << "Hz";
        qDebug() << "  FFT size:" << fft_size;
        qDebug() << "  Frequency resolution:" << frequency_resolution_.load() << "Hz";
        
        return true;
        
    } catch (const std::exception& e) {
        qDebug() << "DynamicBandpassFilter: Initialization failed:" << e.what();
        cleanup();
        return false;
    }
}

void DynamicBandpassFilter::configure(const FilterConfig& config) {
    if (!initialized_.load()) return;
    
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_ = config;
    
    // Update SSB parameters
    ssb_carrier_offset_.store(config.ssb_carrier_offset);
    ssb_sharp_cutoff_.store(config.ssb_sharp_cutoff);
    
    parameters_changed_.store(true);
    
    qDebug() << "DynamicBandpassFilter: Configuration updated (will apply on next process)";
}

void DynamicBandpassFilter::setEnabled(bool enabled) {
    enabled_.store(enabled);
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    current_stats_.is_enabled = enabled;
    
    qDebug() << "DynamicBandpassFilter:" << (enabled ? "Enabled" : "Disabled");
}

bool DynamicBandpassFilter::isValidForProcessing() const {
    return initialized_.load() && 
           enabled_.load() && 
           fft_size_.load() > 0 && 
           frequency_resolution_.load() > 0.0f;
}

void DynamicBandpassFilter::safelyUpdateKernel() {
    if (!isValidForProcessing()) return;
    
    std::lock_guard<std::mutex> filter_lock(filter_mutex_);
    
    int fft_size = fft_size_.load();
    if (filter_kernel_.size() != static_cast<size_t>(fft_size)) {
        filter_kernel_.resize(fft_size, std::complex<float>(1.0f, 0.0f));
    }
    
    float low_cutoff = passband_low_hz_.load() + current_center_freq_.load();
    float high_cutoff = passband_high_hz_.load() + current_center_freq_.load();
    float freq_res = frequency_resolution_.load();
    float carrier_offset = ssb_carrier_offset_.load();
    
    FilterConfig config_copy;
    {
        std::lock_guard<std::mutex> config_lock(config_mutex_);
        config_copy = config_;
    }
    
    // Apply carrier offset for SSB modes
    if (config_copy.protocol == USB || config_copy.protocol == LSB) {
        low_cutoff += carrier_offset;
        high_cutoff += carrier_offset;
    }
    
    for (int i = 0; i < fft_size; ++i) {
        // Convert FFT bin to frequency
        float freq;
        if (i <= fft_size / 2) {
            freq = i * freq_res;
        } else {
            freq = (i - fft_size) * freq_res;
        }
        
        float response = 0.0f;
        
        // SSB-specific filter shaping
        if (config_copy.protocol == USB || config_copy.protocol == LSB) {
            if (ssb_sharp_cutoff_.load()) {
                // Sharp cutoff for SSB with steeper transition
                float transition = freq_res * 1.0f;  // Narrower transition for SSB
                
                if (freq >= low_cutoff && freq <= high_cutoff) {
                    response = 1.0f;
                } else if (freq >= low_cutoff - transition && freq < low_cutoff) {
                    // Steeper transition
                    float t = (freq - (low_cutoff - transition)) / transition;
                    response = t * t * (3.0f - 2.0f * t);  // Smooth step function
                } else if (freq > high_cutoff && freq <= high_cutoff + transition) {
                    float t = ((high_cutoff + transition) - freq) / transition;
                    response = t * t * (3.0f - 2.0f * t);  // Smooth step function
                }
                
                // Additional suppression of opposite sideband for SSB
                if (config_copy.protocol == USB && freq < current_center_freq_.load()) {
                    response *= 0.01f;  // Strong suppression of LSB for USB
                } else if (config_copy.protocol == LSB && freq > current_center_freq_.load()) {
                    response *= 0.01f;  // Strong suppression of USB for LSB
                }
            } else {
                // Standard rectangular filter
                if (freq >= low_cutoff && freq <= high_cutoff) {
                    response = 1.0f;
                }
            }
        } else {
            // Standard filter for non-SSB modes
            if (freq >= low_cutoff && freq <= high_cutoff) {
                response = 1.0f;
            } else {
                // Add transition zone
                float transition = freq_res * 2.0f;
                if (freq >= low_cutoff - transition && freq < low_cutoff) {
                    response = (freq - (low_cutoff - transition)) / transition;
                } else if (freq > high_cutoff && freq <= high_cutoff + transition) {
                    response = ((high_cutoff + transition) - freq) / transition;
                }
            }
        }
        
        // Apply stopband attenuation
        float min_response = std::pow(10.0f, -static_cast<float>(config_copy.stopband_attenuation) / 20.0f);
        response = std::max(response, min_response);
        
        filter_kernel_[i] = std::complex<float>(response, 0.0f);
    }
}

std::vector<std::complex<float>> DynamicBandpassFilter::process(const std::vector<std::complex<float>>& input) {
    if (!isValidForProcessing() || input.empty()) {
        return input;  // Bypass if not ready
    }
    
    // Set processing flag
    processing_active_.store(true);
    
    // Ensure we clear the flag when done
    struct ProcessingGuard {
        std::atomic<bool>& flag;
        ProcessingGuard(std::atomic<bool>& f) : flag(f) {}
        ~ProcessingGuard() { flag.store(false); }
    } guard(processing_active_);
    
    // Simple bypass for very large inputs to prevent memory issues
    int current_fft_size = fft_size_.load();
    if (input.size() > static_cast<size_t>(current_fft_size) * 10) {
        qDebug() << "DynamicBandpassFilter: Input too large, bypassing";
        return input;
    }
    
    // Update filter if parameters changed
    if (parameters_changed_.load()) {
        updateFilterParameters();
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        std::vector<std::complex<float>> output;
        output.reserve(input.size());
        
        size_t pos = 0;
        while (pos < input.size() && isValidForProcessing()) {
            size_t block_size = std::min(static_cast<size_t>(current_fft_size), input.size() - pos);
            
            // Process block with proper locking
            {
                std::lock_guard<std::mutex> fft_lock(fft_mutex_);
                
                if (!fft_input_ || !fft_output_ || !forward_plan_ || !inverse_plan_) {
                    qDebug() << "DynamicBandpassFilter: FFT resources not available";
                    break;
                }
                
                // Clear and fill FFT input
                memset(fft_input_, 0, sizeof(fftwf_complex) * current_fft_size);
                for (size_t i = 0; i < block_size; ++i) {
                    fft_input_[i][0] = input[pos + i].real();
                    fft_input_[i][1] = input[pos + i].imag();
                }
                
                // Forward FFT
                fftwf_execute(forward_plan_);
                
                // Apply filter
                {
                    std::lock_guard<std::mutex> filter_lock(filter_mutex_);
                    if (filter_kernel_.size() == static_cast<size_t>(current_fft_size)) {
                        for (int i = 0; i < current_fft_size; ++i) {
                            std::complex<float> sample(fft_output_[i][0], fft_output_[i][1]);
                            std::complex<float> filtered = sample * filter_kernel_[i];
                            fft_output_[i][0] = filtered.real();
                            fft_output_[i][1] = filtered.imag();
                        }
                    }
                }
                
                // Inverse FFT
                fftwf_execute(inverse_plan_);
                
                // Extract results with normalization
                float norm = 1.0f / current_fft_size;
                for (size_t i = 0; i < block_size; ++i) {
                    output.emplace_back(fft_input_[i][0] * norm, fft_input_[i][1] * norm);
                }
            }
            
            pos += block_size;
        }
        
        // Update statistics
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            current_stats_.processing_time_ms = duration.count() / 1000.0f;
            current_stats_.samples_processed = input.size();
            total_samples_processed_.fetch_add(input.size());
        }
        
        return output;
        
    } catch (const std::exception& e) {
        qDebug() << "DynamicBandpassFilter: Processing exception:" << e.what();
        return input;
    } catch (...) {
        qDebug() << "DynamicBandpassFilter: Unknown processing exception";
        return input;
    }
}

void DynamicBandpassFilter::processInPlace(std::vector<std::complex<float>>& samples) {
    if (!isValidForProcessing() || samples.empty()) {
        return;
    }
    
    try {
        samples = process(samples);
    } catch (...) {
        qDebug() << "DynamicBandpassFilter: processInPlace exception";
    }
}

void DynamicBandpassFilter::setProtocol(Protocol protocol) {
    if (!initialized_.load()) return;
    
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    if (config_.protocol == protocol) {
        return;
    }
    
    config_.protocol = protocol;
    
    // Apply protocol defaults
    const auto& defaults = PROTOCOL_DEFAULTS[static_cast<int>(protocol)];
    float half_bandwidth = defaults.passband_width / 2.0f;
    passband_low_hz_.store(-half_bandwidth);
    passband_high_hz_.store(half_bandwidth);
    config_.bandwidth = defaults.passband_width;
    config_.stopband_attenuation = defaults.stopband_atten;
    config_.ssb_carrier_offset = defaults.carrier_offset;
    config_.ssb_sharp_cutoff = defaults.sharp_cutoff;
    
    // Update atomic values
    ssb_carrier_offset_.store(defaults.carrier_offset);
    ssb_sharp_cutoff_.store(defaults.sharp_cutoff);
    
    parameters_changed_.store(true);
    
    // Update statistics
    {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        current_stats_.ssb_mode_active = (protocol == USB || protocol == LSB);
        current_stats_.ssb_carrier_offset_hz = defaults.carrier_offset;
    }
    
    qDebug() << "DynamicBandpassFilter: Protocol changed to" << protocol_names[protocol];
    if (protocol == USB || protocol == LSB) {
        qDebug() << "  SSB carrier offset:" << defaults.carrier_offset << "Hz";
        qDebug() << "  Sharp cutoff:" << (defaults.sharp_cutoff ? "enabled" : "disabled");
    }
}

void DynamicBandpassFilter::setPassband(float low_freq, float high_freq) {
    if (low_freq >= high_freq || !initialized_.load()) {
        return;
    }
    
    passband_low_hz_.store(low_freq);
    passband_high_hz_.store(high_freq);
    parameters_changed_.store(true);
    
    qDebug() << "DynamicBandpassFilter: Passband set to" << low_freq << "Hz to" << high_freq << "Hz";
}

void DynamicBandpassFilter::setCenterFrequency(float center_freq) {
    if (!initialized_.load()) return;
    
    current_center_freq_.store(center_freq);
    
    {
        std::lock_guard<std::mutex> lock(config_mutex_);
        config_.center_frequency = center_freq;
    }
    
    parameters_changed_.store(true);
    
    qDebug() << "DynamicBandpassFilter: Center frequency set to" << center_freq << "Hz";
}

void DynamicBandpassFilter::setSSBCarrierOffset(float offset_hz) {
    if (!initialized_.load()) return;
    
    ssb_carrier_offset_.store(offset_hz);
    
    {
        std::lock_guard<std::mutex> lock(config_mutex_);
        config_.ssb_carrier_offset = offset_hz;
    }
    
    parameters_changed_.store(true);
    
    {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        current_stats_.ssb_carrier_offset_hz = offset_hz;
    }
    
    qDebug() << "DynamicBandpassFilter: SSB carrier offset set to" << offset_hz << "Hz";
}

void DynamicBandpassFilter::setSSBSharpCutoff(bool enabled) {
    if (!initialized_.load()) return;
    
    ssb_sharp_cutoff_.store(enabled);
    
    {
        std::lock_guard<std::mutex> lock(config_mutex_);
        config_.ssb_sharp_cutoff = enabled;
    }
    
    parameters_changed_.store(true);
    
    qDebug() << "DynamicBandpassFilter: SSB sharp cutoff" << (enabled ? "enabled" : "disabled");
}

float DynamicBandpassFilter::getSSBCarrierOffset() const {
    return ssb_carrier_offset_.load();
}

bool DynamicBandpassFilter::isSSBMode() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return (config_.protocol == USB || config_.protocol == LSB);
}

float DynamicBandpassFilter::getResponse(float frequency) const {
    if (!isValidForProcessing()) {
        return 1.0f;
    }
    
    std::lock_guard<std::mutex> filter_lock(filter_mutex_);
    
    if (filter_kernel_.empty()) {
        return 1.0f;
    }
    
    // Simple frequency to bin conversion
    float sample_rate;
    {
        std::lock_guard<std::mutex> config_lock(config_mutex_);
        sample_rate = static_cast<float>(config_.sample_rate);
    }
    
    float nyquist = sample_rate / 2.0f;
    if (std::abs(frequency) > nyquist) {
        return 0.0f;
    }
    
    float freq_res = frequency_resolution_.load();
    int fft_size = fft_size_.load();
    
    int bin = static_cast<int>((frequency + nyquist) / freq_res);
    bin = std::clamp(bin, 0, fft_size - 1);
    
    if (bin < static_cast<int>(filter_kernel_.size())) {
        return std::abs(filter_kernel_[bin]);
    }
    
    return 1.0f;
}

DynamicBandpassFilter::FilterConfig DynamicBandpassFilter::getConfiguration() {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return config_;
}

DynamicBandpassFilter::FilterStats DynamicBandpassFilter::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    FilterStats stats = current_stats_;
    stats.passband_width_hz = passband_high_hz_.load() - passband_low_hz_.load();
    stats.current_center_freq = current_center_freq_.load();
    stats.ssb_carrier_offset_hz = ssb_carrier_offset_.load();
    
    {
        std::lock_guard<std::mutex> config_lock(config_mutex_);
        stats.stopband_attenuation_db = config_.stopband_attenuation;
        stats.ssb_mode_active = (config_.protocol == USB || config_.protocol == LSB);
    }
    
    return stats;
}

void DynamicBandpassFilter::reset() {
    if (!initialized_.load()) return;
    
    // Wait for processing to complete
    while (processing_active_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    std::fill(energy_history_.begin(), energy_history_.end(), 0.0f);
    energy_history_idx_ = 0;
    
    {
        std::lock_guard<std::mutex> config_lock(config_mutex_);
        current_center_freq_.store(config_.center_frequency);
        ssb_carrier_offset_.store(config_.ssb_carrier_offset);
    }
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        total_samples_processed_.store(0);
        current_stats_.samples_processed = 0;
        current_stats_.processing_time_ms = 0.0f;
    }
    
    qDebug() << "DynamicBandpassFilter: Reset completed";
}

void DynamicBandpassFilter::designFilter() {
    if (!isValidForProcessing()) return;
    
    // Use SSB-specific filter design if in SSB mode
    Protocol current_protocol;
    {
        std::lock_guard<std::mutex> config_lock(config_mutex_);
        current_protocol = config_.protocol;
    }
    
    if (current_protocol == USB || current_protocol == LSB) {
        designSSBFilter();
    } else {
        safelyUpdateKernel();
    }
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        current_stats_.passband_width_hz = passband_high_hz_.load() - passband_low_hz_.load();
        current_stats_.current_center_freq = current_center_freq_.load();
        current_stats_.ssb_carrier_offset_hz = ssb_carrier_offset_.load();
        
        std::lock_guard<std::mutex> config_lock(config_mutex_);
        current_stats_.stopband_attenuation_db = config_.stopband_attenuation;
        current_stats_.ssb_mode_active = (config_.protocol == USB || config_.protocol == LSB);
    }
    
    float low_cutoff = passband_low_hz_.load() + current_center_freq_.load();
    float high_cutoff = passband_high_hz_.load() + current_center_freq_.load();
    float carrier_offset = ssb_carrier_offset_.load();
    
    qDebug() << "DynamicBandpassFilter: Filter designed";
    qDebug() << "  Passband:" << low_cutoff << "Hz to" << high_cutoff << "Hz";
    if (current_protocol == USB || current_protocol == LSB) {
        qDebug() << "  SSB carrier offset:" << carrier_offset << "Hz";
        qDebug() << "  Effective passband:" << (low_cutoff + carrier_offset) << "Hz to" << (high_cutoff + carrier_offset) << "Hz";
    }
}

void DynamicBandpassFilter::designSSBFilter() {
    if (!isValidForProcessing()) return;
    
    safelyUpdateKernel();  // Use the enhanced kernel design that handles SSB
    
    // Apply additional SSB-specific shaping if needed
    int fft_size = fft_size_.load();
    float freq_res = frequency_resolution_.load();
    applySSBShaping(fft_size, freq_res);
}

void DynamicBandpassFilter::applySSBShaping(int fft_size, float freq_res) {
    std::lock_guard<std::mutex> filter_lock(filter_mutex_);
    
    if (filter_kernel_.size() != static_cast<size_t>(fft_size)) {
        return;
    }
    
    Protocol current_protocol;
    {
        std::lock_guard<std::mutex> config_lock(config_mutex_);
        current_protocol = config_.protocol;
    }
    
    if (current_protocol != USB && current_protocol != LSB) {
        return;
    }
    
    float center_freq = current_center_freq_.load();
    
    // Additional shaping for SSB to ensure clean sideband suppression
    for (int i = 0; i < fft_size; ++i) {
        float freq;
        if (i <= fft_size / 2) {
            freq = i * freq_res;
        } else {
            freq = (i - fft_size) * freq_res;
        }
        
        // Enhanced opposite sideband suppression
        float suppression_factor = 1.0f;
        
        if (current_protocol == USB && freq < center_freq) {
            // Suppress lower sideband for USB
            float distance = std::abs(freq - center_freq);
            if (distance < 3000.0f) {  // Within 3kHz of center
                suppression_factor = 0.001f;  // -60dB suppression
            } else {
                suppression_factor = 0.1f;   // -20dB suppression for far frequencies
            }
        } else if (current_protocol == LSB && freq > center_freq) {
            // Suppress upper sideband for LSB
            float distance = std::abs(freq - center_freq);
            if (distance < 3000.0f) {  // Within 3kHz of center
                suppression_factor = 0.001f;  // -60dB suppression
            } else {
                suppression_factor = 0.1f;   // -20dB suppression for far frequencies
            }
        }
        
        filter_kernel_[i] *= suppression_factor;
    }
}


void DynamicBandpassFilter::createWindow(int size, FilterShape shape, std::vector<float>& window) {
    window.resize(size);
    
    switch (shape) {
        case RECTANGULAR:
            std::fill(window.begin(), window.end(), 1.0f);
            break;
            
        case HAMMING:
            for (int i = 0; i < size; ++i) {
                window[i] = 0.54f - 0.46f * cosf(2.0f * M_PI * i / (size - 1));
            }
            break;
            
        case BLACKMAN:
            for (int i = 0; i < size; ++i) {
                float arg = 2.0f * M_PI * i / (size - 1);
                window[i] = 0.42f - 0.5f * cosf(arg) + 0.08f * cosf(2.0f * arg);
            }
            break;
            
        case KAISER:
        {
            // Simplified Kaiser window implementation
            float beta = calculateKaiserBeta(60.0f); // Default attenuation
            for (int i = 0; i < size; ++i) {
                float x = 2.0f * i / (size - 1) - 1.0f;
                window[i] = std::cosh(beta * std::sqrt(1.0f - x * x)) / std::cosh(beta);
            }
            break;
        }
            
        default:
            std::fill(window.begin(), window.end(), 1.0f);
            break;
    }
}

float DynamicBandpassFilter::calculateKaiserBeta(float attenuation_db) {
    if (attenuation_db > 50.0f) {
        return 0.1102f * (attenuation_db - 8.7f);
    } else if (attenuation_db >= 21.0f) {
        return 0.5842f * std::pow(attenuation_db - 21.0f, 0.4f) + 0.07886f * (attenuation_db - 21.0f);
    } else {
        return 0.0f;
    }
}

void DynamicBandpassFilter::updateAdaptiveCentering(const std::vector<std::complex<float>>& spectrum) {
    // Disabled for simplicity - could be enhanced for SSB auto-tuning
    (void)spectrum;
}

void DynamicBandpassFilter::cleanup() {
    // This should only be called from destructor or when we have exclusive access
    
    if (forward_plan_) {
        fftwf_destroy_plan(forward_plan_);
        forward_plan_ = nullptr;
    }
    
    if (inverse_plan_) {
        fftwf_destroy_plan(inverse_plan_);
        inverse_plan_ = nullptr;
    }
    
    if (fft_input_) {
        fftwf_free(fft_input_);
        fft_input_ = nullptr;
    }
    
    if (fft_output_) {
        fftwf_free(fft_output_);
        fft_output_ = nullptr;
    }
    
    {
        std::lock_guard<std::mutex> filter_lock(filter_mutex_);
        filter_kernel_.clear();
    }
    
    energy_history_.clear();
    
    initialized_.store(false);
}

void DynamicBandpassFilter::updateFilterParameters() {
    if (!isValidForProcessing()) return;
    
    if (!parameters_changed_.load()) return;
    
    try {
        // Update passband from protocol if needed
        {
            std::lock_guard<std::mutex> config_lock(config_mutex_);
            const auto& defaults = PROTOCOL_DEFAULTS[static_cast<int>(config_.protocol)];
            float half_bandwidth = defaults.passband_width / 2.0f;
            
            // Only auto-update if symmetric (except for SSB modes)
            float current_low = passband_low_hz_.load();
            float current_high = passband_high_hz_.load();
            
            if (config_.protocol == USB || config_.protocol == LSB) {
                // SSB modes may have asymmetric passbands
                // Update carrier offset and sharp cutoff settings
                ssb_carrier_offset_.store(config_.ssb_carrier_offset);
                ssb_sharp_cutoff_.store(config_.ssb_sharp_cutoff);
            } else if (std::abs(current_low + current_high) < 1.0f) {
                // Symmetric passband update for non-SSB modes
                passband_low_hz_.store(-half_bandwidth);
                passband_high_hz_.store(half_bandwidth);
            }
        }
        
        designFilter();
        parameters_changed_.store(false);
        
        qDebug() << "DynamicBandpassFilter: Parameters updated";
        
    } catch (const std::exception& e) {
        qDebug() << "DynamicBandpassFilter: Parameter update failed:" << e.what();
        parameters_changed_.store(false);
    }
}