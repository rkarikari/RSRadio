#ifndef DYNAMIC_BANDPASS_FILTER_H
#define DYNAMIC_BANDPASS_FILTER_H

#include <complex>
#include <vector>
#include <atomic>
#include <mutex>
#include <fftw3.h>

class DynamicBandpassFilter {
public:
    enum Protocol {
        WFM,
        NBFM,
        AM,
        USB,    // Added USB support
        LSB     // Added LSB support
    };

    enum FilterShape {
        RECTANGULAR,
        HAMMING,
        BLACKMAN,
        KAISER
    };

    struct FilterConfig {
        Protocol protocol;
        FilterShape shape;
        double stopband_attenuation;
        double center_frequency;
        double bandwidth;
        double sample_rate;
        // SSB-specific parameters
        double ssb_carrier_offset;  // Offset from center frequency for SSB carrier
        bool ssb_sharp_cutoff;      // Enable sharper cutoff for SSB
        // Add other config parameters as needed
    };

    struct FilterStats {
        double frequency_response;
        double attenuation;
        bool is_active;
        bool is_enabled;
        size_t samples_processed;
        double passband_width_hz;
        double stopband_attenuation_db;
        double processing_time_ms;
        double current_center_freq;
        // SSB-specific stats
        double ssb_carrier_offset_hz;
        bool ssb_mode_active;
        // Add other stats as needed
    };

    // Constructor
    DynamicBandpassFilter();
    ~DynamicBandpassFilter();

    // Delete copy constructor and assignment operator to prevent issues
    DynamicBandpassFilter(const DynamicBandpassFilter&) = delete;
    DynamicBandpassFilter& operator=(const DynamicBandpassFilter&) = delete;

    // Initialization
    bool initialize(int sample_rate, int fft_size);
    bool isInitialized() const { 
        std::lock_guard<std::mutex> lock(state_mutex_);
        return initialized_; 
    }

    // Protocol and configuration
    void setProtocol(Protocol protocol);
    void setEnabled(bool enabled);
    void configure(const FilterConfig& config);
    void setPassband(float low_freq, float high_freq);
    void setCenterFrequency(float center_freq);
    
    // SSB-specific configuration methods
    void setSSBCarrierOffset(float offset_hz);
    void setSSBSharpCutoff(bool enabled);
    float getSSBCarrierOffset() const;
    bool isSSBMode() const;

    // Processing
    std::vector<std::complex<float>> process(const std::vector<std::complex<float>>& input);
    void processInPlace(std::vector<std::complex<float>>& samples);
    
    // Response and analysis
    float getResponse(float frequency) const;
    FilterConfig getConfiguration();
    
    // Statistics
    FilterStats getStats() const;
    void reset();

private:
    // Core state with proper atomic types
    mutable std::mutex state_mutex_;
    std::atomic<bool> initialized_;
    std::atomic<bool> enabled_;
    std::atomic<bool> parameters_changed_;
    std::atomic<bool> processing_active_;
    
    // Configuration
    FilterConfig config_;
    mutable std::mutex config_mutex_;
    
    // FFT parameters
    std::atomic<int> fft_size_;
    int overlap_size_;
    std::atomic<float> frequency_resolution_;
    
    // FFTW plans and buffers - protected by processing mutex
    mutable std::mutex fft_mutex_;
    fftwf_plan forward_plan_;
    fftwf_plan inverse_plan_;
    fftwf_complex* fft_input_;
    fftwf_complex* fft_output_;
    
    // Filter parameters
    std::vector<std::complex<float>> filter_kernel_;
    mutable std::mutex filter_mutex_;
    std::atomic<float> passband_low_hz_;
    std::atomic<float> passband_high_hz_;
    std::atomic<float> current_center_freq_;
    
    // SSB-specific parameters
    std::atomic<float> ssb_carrier_offset_;
    std::atomic<bool> ssb_sharp_cutoff_;
    
    // Adaptive features
    std::vector<float> energy_history_;
    size_t energy_history_idx_;
    float adaptive_alpha_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    FilterStats current_stats_;
    std::atomic<size_t> total_samples_processed_;
    
    // Private methods
    void cleanup();
    void designFilter();
    void updateFilterParameters();
    void createWindow(int size, FilterShape shape, std::vector<float>& window);
    float calculateKaiserBeta(float attenuation_db);
    void updateAdaptiveCentering(const std::vector<std::complex<float>>& spectrum);
    
    // SSB-specific filter design methods
    void designSSBFilter();
    void applySSBShaping(int fft_size, float freq_res);
    
    // Thread-safe helpers
    bool isValidForProcessing() const;
    void safelyUpdateKernel();
};

#endif // DYNAMIC_BANDPASS_FILTER_H