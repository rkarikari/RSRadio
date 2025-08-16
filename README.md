# RadioSport SDR 

**Version:** 1.6  
**Build:** 2025.11.08  
**Portable Edition - No Installation Required**
<img width="1632" height="1135" alt="start" src="https://github.com/user-attachments/assets/a410c9ae-33b4-45f6-b3db-07090fa5db8a" />

---

## Table of Contents

1. [Quick Start Guide](#quick-start-guide)
2. [Portable Installation](#portable-installation)
3. [Main Interface Overview](#main-interface-overview)
4. [Tabbed Control Interface](#tabbed-control-interface)
5. [Audio Protocols & Modes](#audio-protocols--modes)
6. [Advanced Features](#advanced-features)
7. [Session State Management](#session-state-management)
8. [Performance Optimization](#performance-optimization)
9. [Troubleshooting](#troubleshooting)
10. [Technical Specifications](#technical-specifications)

---

## Quick Start Guide

### Portable Setup (No Installation Required)
1. **Extract Files**: Unzip the RadioSport SDR package to any location (USB drive, Desktop, etc.)
2. **Connect Hardware**: Plug your RTL-SDR dongle into a USB port
3. **Run Application**: Double-click the `Radio` shortcut , - no installation needed!
4. **First Use**: The app will auto-detect your RTL-SDR device and apply optimal default settings

### 30-Second Radio Reception
1. **Start Reception**: Click the green "START" button
2. **Select Protocol**: Choose "WFM" for FM radio (default)
3. **Set Frequency**: Enter 87.9 MHz in the frequency box
4. **Adjust Volume**: Use the volume slider to comfortable level
5. **Fine-Tune**: Watch the spectrum display and adjust frequency for best signal

---

## Portable Installation

### System Requirements
- **Windows**: Windows 10/11 (64-bit recommended)
- **USB**: Any available USB 2.0 or 3.0 port
- **RTL-SDR**: Any RTL2832U-based USB dongle
- **Storage**: 50MB free space (works from USB drive)
- **RAM**: 4GB minimum, 8GB recommended



### Running from USB Drive
- **Plug & Play**: Copy entire folder to USB drive
- **No Admin Rights**: Runs without administrator privileges
- **Portable Settings**: All configurations saved in local settings folder
- **Cross-Computer**: Move USB drive between computers seamlessly

---

## Main Interface Overview
<img width="1622" height="1121" alt="pic1" src="https://github.com/user-attachments/assets/9f2c8aed-25fc-418c-a237-459843285b6c" />


*Main interface showing RF spectrum display with FM radio signal *

### Header Section
- **Title Bar**: "RadioSport SDR" with version and callsign (9G5AR)
- **Status Indicator**: Real-time connection status (STOPPED/RECEIVING)
- **Signal Meter**: Live signal strength in dBFS with peak indicators
- **Build Information**: Version and build date display

### Main Layout
- **Left Panel**: Large spectrum display with RF/Audio visualization
- **Right Panel**: Tabbed control interface with 5 organized sections
- **Status Bar**: Real-time system messages and current operations
- **Power Control**: Prominent START/STOP button always visible

### Spectrum Display Features

The spectrum display can show two different views:

#### RF Spectrum (Raw SDR)

*RF spectrum showing raw SDR data with orange trace*

- **Orange Trace**: Raw RF spectrum from the SDR hardware
- **Frequency Marker**: Yellow line shows current tuning position (87.900 MHz)
- **Signal Peaks**: Strong FM broadcast signals visible
- **Dynamic Range**: -122 dB to +10 dB scale

#### Audio Spectrum (Processed)

<img width="1620" height="1130" alt="pic5" src="https://github.com/user-attachments/assets/d66886f8-cc06-4f78-a3c6-e236c86192d8" />

*Audio spectrum showing processed demodulated signal with green trace*

- **Green Trace**: Processed audio spectrum after demodulation
- **Cleaner Display**: Shows demodulated audio content
- **Audio Bandwidth**: Filtered to audio frequency range
- **Real-Time Updates**: Smooth 30 FPS display refresh

### Key Interface Elements Visible:
- **Frequency Control**: 87.9000 MHz with 100 kHz step size
- **Decimation Factor**: 1× (250 → 250 kHz) for maximum bandwidth
- **Audio Volume**: 8% setting with slider control
- **FM Presets**: Quick access buttons for common FM frequencies
- **Signal Strength**: Live dBFS readings in header

---

## Tabbed Control Interface

The right panel contains five tabs for different control functions:

### Tab 1: Main Controls


<img width="574" height="901" alt="main" src="https://github.com/user-attachments/assets/12262c39-4d8f-4776-bb4e-6687a986eadf" />

#### Frequency Management
- **Primary Input**: High-precision frequency entry (0.5-1750 MHz)
- **Step Control**: Smart stepping button cycles through:
  - 100 kHz (Normal) - Coarse tuning
  - 10 kHz (Medium) - Standard fine tuning  
  - 1 kHz (Fine) - Precise adjustments
  - 0.1 kHz (Extra Fine) - Ultra-precise tuning
- **Keyboard Navigation**: Arrow keys and mouse wheel with active step size
- **FM Presets**: Quick access to common FM frequencies (87.9, 90.1, 95.7, 101.1, 105.7, 107.9 MHz)

#### Sample Rate Control
- **Decimation Factor**: Optimizes CPU usage vs bandwidth
  - 1× (250→250 kHz) - Maximum bandwidth, higher CPU
  - 4× (1000→250 kHz) - Balanced performance
  - 6× (1500→250 kHz) - Good performance
  - 8× (2000→250 kHz) - Lower CPU usage

#### Audio Volume
- **Range**: 0-100% with soft limiting at 95%
- **Real-Time**: Instant adjustment with no audio dropouts
- **Protection**: Automatic clipping prevention

### Tab 2: Gain Control

<img width="572" height="885" alt="Tab 2" src="https://github.com/user-attachments/assets/bb75cd41-47ee-482b-b45a-d94d77cc8180" />

*Gain control tab showing RF gain slider, AGC settings, and squelch control*

#### RF Gain Management
- **Manual Control**: 0-50 dB RF gain slider (shown at 33 dB)
- **Quick Presets**: Low (15dB), Medium (30dB), High (45dB)
- **Real-Time Display**: Current gain value with dB indication

#### Frequency Correction (PPM) ####
- Corrects the crystal oscilator frequency errors.
- Choose an signal of known (accurate) frequency and Adjust to match.
- Use the slider or presets.


#### Advanced AGC System
Four sophisticated AGC modes for optimal signal handling:

1. **Disabled**: Complete manual control
   - User controls all gain settings
   - Best for consistent signal environments

2. **Audio AGC Only**: 
   - Automatic audio level control
   - Manual RF gain adjustment
   - Good for varying audio levels

3. **SDR AGC Only**:
   - Hardware-level automatic gain control
   - Fixed audio processing
   - Excellent for weak/strong signal transitions

4. **Full AGC (Both)**: *(Currently selected in screenshot)*
   - Coordinated dual-AGC system
   - Hardware + software AGC working together
   - Optimal for general-purpose reception
   - Status: "Full AGC - Both RF and Audio automatic"

#### Squelch System
- **Spectrum-Based**: Uses peak spectrum analysis (not simple level)
- **Threshold Range**: -120 to -40 dBFS (shown at -120 dBFS)
- **Smart Hysteresis**: 5dB deadband prevents chattering
- **Fast Response**: Real-time spectrum monitoring
- **Enable/Disable**: Checkbox control for squelch activation

### Tab 3: Mode Selection

<img width="581" height="897" alt="pic3" src="https://github.com/user-attachments/assets/1cd4ea23-572b-4eb7-b4ea-c6c0620717b8" />


#### Protocol Selection
Comprehensive demodulation support:

- **WFM (Wideband FM)**:
  - Commercial FM radio (87.5-108 MHz)
  - Full stereo support with 15kHz audio bandwidth
  - 75μs de-emphasis for broadcast standard

- **NBFM (Narrowband FM)**:
  - VHF/UHF communications (144-446 MHz typical)
  - 3kHz audio bandwidth optimized for voice
  - 50μs de-emphasis for communications standard

- **AM (Amplitude Modulation)**:
  - AM broadcast (530-1700 kHz) and aviation (118-137 MHz)
  - 5kHz audio bandwidth with envelope detection
  - Carrier tracking for stable demodulation

- **USB (Upper Sideband)**:
  - HF amateur radio above 10 MHz
  - 3kHz audio bandwidth with product detection
  - Adjustable carrier frequency (500-3000 Hz)

- **LSB (Lower Sideband)**:
  - HF amateur radio below 10 MHz  
  - 3kHz audio bandwidth with product detection
  - Hilbert transform processing

#### Quick Selection Buttons
- **Visual Feedback**: Selected mode highlighted in accent color
- **Instant Switching**: One-click mode changes
- **Smart Defaults**: Each mode applies optimal settings automatically

#### Frequency Suggestions
Context-aware frequency recommendations based on selected protocol:
- **WFM**: Commercial FM stations (87.9, 90.1, 95.7, etc.)
- **NBFM**: Amateur repeaters (145.5), NOAA weather (162.4), business (460.1)
- **AM**: Broadcast (530, 810, 1160 kHz), aviation (119.5, 122.6 MHz)
- **USB**: Amateur HF bands (14.205, 21.305 MHz)
- **LSB**: Lower HF bands (3.795, 7.195 MHz)

### Tab 4: Noise Reduction (NR)

![Noise<img width="574" height="903" alt="pic4" src="https://github.com/user-attachments/assets/6f200eee-4dea-4b5b-a7c9-242470e417e8" />

*Noise Reduction control panel showing spectral subtraction algorithm settings*

#### Advanced Noise Reduction System
Three sophisticated algorithms for different noise environments:

1. **Spectral Subtraction**: *(Currently selected in screenshot)*
   - Best for constant background noise (fan noise, hum)
   - Multi-band processing with over-subtraction control
   - Excellent for steady-state interference

2. **Spectral Gating**:
   - Ideal for intermittent noise and weak signals
   - Voice activity detection with adaptive thresholds
   - Perfect for voice communications

3. **Adaptive Wiener Filtering**:
   - Superior for variable noise conditions
   - Real-time noise estimation and adaptation
   - Best all-around algorithm for changing environments

#### Control Parameters (as shown in screenshot)
- **Enable Checkbox**: "Enable Noise Reduction" (currently unchecked)
- **Algorithm Selection**: Dropdown showing "Spectral Subtraction"
- **Intensity**: 50% setting (blue slider)
- **Threshold**: -40 dB setting (blue slider)
- **Quick Presets**: Light, Medium, Strong, Max buttons
- **Reset Profile**: Orange button to clear learned noise patterns

#### Status Display
- **Status**: "Disabled" (shown in screenshot)
- **Noise Level**: "--- dB" (no measurement when disabled)
- **Learning Indicator**: Shows when algorithm is adapting
- **Processing Status**: Active/Learning/Disabled states

#### Quick Presets
- **Light (25%, -30dB)**: Minimal processing for clean signals
- **Medium (50%, -40dB)**: Balanced noise reduction
- **Strong (75%, -50dB)**: Aggressive noise suppression
- **Max (90%, -55dB)**: Maximum noise reduction

### Tab 5: Filter Control


<img width="579" height="906" alt="audio" src="https://github.com/user-attachments/assets/98f99be9-3b4f-45be-8408-8880055695b7" />

#### High-Precision Bandpass Filter
- **Protocol-Adaptive**: Automatically adjusts for selected demodulation mode
- **Specifications by Mode**:
  - WFM: 15kHz audio BW, 200kHz RF BW
  - NBFM: 3kHz audio BW, 25kHz RF BW  
  - AM: 5kHz audio BW, 10kHz RF BW
  - USB/LSB: 3kHz audio BW, 3kHz RF BW

#### Filter Status Monitoring
- **Real-Time Display**: Current filter state and parameters
- **Performance Metrics**: Processing load and filter health
- **Adaptive Response**: Automatic adjustment to changing conditions

---

## Audio Protocols & Modes

### Understanding the Spectrum Displays

The application provides two complementary views of signal activity:


### Protocol-Specific Operation

#### Wideband FM (WFM) - Currently Active
Based on the screenshots showing 87.9 MHz operation:
- **Frequency Range**: 87.5-108 MHz (FM broadcast band)
- **Audio Bandwidth**: 15 kHz (full fidelity stereo)
- **Typical Signal Strength**: -11 to -18.9 dBFS for strong local stations
- **Deemphasis**: 75μs standard for broadcast
- **Applications**: Commercial FM radio, high-quality music reception

---

## Advanced Features

### Real-Time Performance Monitoring

The status bar shows critical system information:
- **AGC Status**: "SDR AGC Active - Software AGC: RMS=0.336, Target=0.700"
- **Signal Measurements**: Continuous dBFS readings with peak detection
- **Processing Load**: Real-time CPU and buffer monitoring

### Keyboard Shortcuts
- **Ctrl+F**: Toggle frequency step size (cycles through 100k/10k/1k/0.1k)
- **Arrow Keys**: Fine frequency adjustment using current step size
- **Mouse Wheel**: Smooth frequency tuning (over frequency display)
- **Tab Navigation**: Quick access to different control sections

### Multi-Threading Architecture
- **Separate RF Thread**: RTL-SDR data acquisition
- **Audio Processing Thread**: Demodulation and filtering
- **UI Thread**: Display updates and user interaction
- **Background Processing**: Noise reduction and spectrum analysis

### Advanced Audio Processing
- **32-bit Floating Point**: Professional-grade audio precision
- **FFTW3 Acceleration**: Optimized FFT processing
- **Multistage Filtering**: Protocol-specific filter chains
- **AGC Coordination**: Hardware/software AGC cooperation

---

## Session State Management

### Automatic State Persistence
RadioSport SDR automatically remembers your settings between sessions:

#### Saved Settings Include:
- **Window Configuration**: Size, position, and layout
- **RF Parameters**: Frequency, gain, decimation factor
- **Audio Settings**: Volume, protocol, AGC mode
- **Noise Reduction**: Algorithm, intensity, threshold, enable state
- **Squelch Configuration**: Enabled state and threshold level
- **Filter Settings**: Bandpass filter enable state
- **SSB Parameters**: Carrier frequency for USB/LSB modes
- **Display Mode**: Spectrum display preference (RF vs Audio)


### Session Recovery
- **Automatic Loading**: Previous session restored on startup
- **Graceful Degradation**: Invalid settings reset to safe defaults
- **Fresh Start Option**: Delete settings folder for factory reset

---

---

## Troubleshooting

### Common Issues and Solutions

#### "No RTL-SDR devices found"
**Symptoms**: Application can't detect RTL-SDR dongle
**Solutions**:
1. Check USB connection - try different USB port
2. Install RTL-SDR drivers 
3. Ensure dongle not in use by other software
4. Try USB 2.0 port if USB 3.0 causes issues
5. Run Windows Device Manager to check for hardware conflicts

#### No Audio Output
**Symptoms**: Spectrum shows signal but no sound
**Solutions**:
1. Check volume slider (not muted/zero) - ensure above 5%
2. Verify squelch is disabled or threshold set correctly (-120 dBFS as shown)
3. Confirm correct protocol for frequency (WFM for FM radio)
4. Test with strong known signal (local FM station like 87.9 MHz)
5. Check Windows audio mixer settings

#### Poor Signal Quality
**Symptoms**: Low signal readings 
**Solutions**:
1. Adjust RF gain - try 45 dB for weak signals
2. Verify antenna connection and placement
3. Use Full AGC mode for automatic optimization
4. Check frequency accuracy using spectrum display
5. Try different location with better reception

#### Interface Issues
**Symptoms**: Tabs not responding or display problems
**Solutions**:
1. Check that correct tab is selected (highlighted in blue)
2. Ensure proper frequency range for selected mode
3. Restart application if interface becomes unresponsive
4. Verify all controls are within valid ranges

---

## Technical Specifications

### RF Processing
- **Frequency Range**: 0.5-1766 MHz (RTL-SDR dependent)
- **Sample Rates**: 250 kHz - 2 MHz baseband (decimation-dependent)
- **Dynamic Range**: >70 dB with proper gain settings
- **Sensitivity**: -120 dBm typical (10 Hz bandwidth)
- **Frequency Accuracy**: ±1 ppm (crystal dependent)

### Audio Processing
- **Output Sample Rate**: 48 kHz (professional quality)
- **Bit Depth**: 32-bit floating point internal processing
- **Frequency Response**: Flat ±0.1dB within passband
- **THD+N**: <0.1% at normal operating levels
- **Processing Latency**: <50ms total system delay

### Demodulation Specifications

#### Wideband FM (WFM)
- **RF Bandwidth**: 200 kHz (-3dB)
- **Audio Bandwidth**: 15 kHz (full fidelity)
- **Stereo Separation**: >40dB at 1 kHz
- **De-emphasis**: 75μs (US/Europe standard)
- **SNR**: >60dB for strong signals

#### Performance Metrics from Screenshots:
- **Signal Range**: -121.6 to -11.0 dBFS demonstrated
- **AGC Response**: RMS=0.334-1.190, Target=0.700
- **Frequency Stability**: ±0.1 kHz accuracy shown
- **Real-time Updates**: 30 FPS spectrum refresh rate

### System Requirements
- **CPU Usage**: 15-50% (modern quad-core system)
- **Memory Usage**: 50-200 MB typical
- **Storage**: 50 MB application, 1 MB settings
- **USB Bandwidth**:  (decimation dependent)
- **Real-time Performance**: Designed for continuous operation

---

## Copyright and Legal

**RadioSport SDR**  
**Copyright © 2025 (RNK). All rights reserved.**

### License Terms
- **Personal Use**: Free for amateur radio and personal use
- **Educational Use**: Permitted in educational environments
- **Commercial Use**: Contact RNK for licensing


### Third-Party Acknowledgments
- **Qt Framework**: Cross-platform application framework
- **RTL-SDR**: Open source RTL2832U driver project
- **PortAudio**: Cross-platform audio I/O library
- **FFTW**: Fast Fourier Transform library
