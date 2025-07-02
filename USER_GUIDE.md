# CV to OSC Converter - User Guide

## Table of Contents
1. [Installation](#installation)
2. [First Run](#first-run)
3. [Configuration](#configuration)
4. [Advanced Features](#advanced-features)
5. [Hardware Setup](#hardware-setup)
6. [Troubleshooting](#troubleshooting)
7. [Performance Tuning](#performance-tuning)

## Installation

### macOS

#### Prerequisites
- macOS 10.14 or later
- Homebrew (install from [brew.sh](https://brew.sh))

#### Step-by-step Installation

1. **Install Homebrew** (if not already installed):
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```

2. **Install dependencies**:
   ```bash
   brew install portaudio liblo nlohmann-json cmake pkg-config
   ```

3. **Clone the repository**:
   ```bash
   git clone https://github.com/your-username/cv_to_osc_converter.git
   cd cv_to_osc_converter
   ```

4. **Build the application**:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```

5. **Test the installation**:
   ```bash
   ./cv_to_osc_converter --help
   ```

### Linux (Ubuntu/Debian)

1. **Install dependencies**:
   ```bash
   sudo apt-get update
   sudo apt-get install libportaudio2-dev liblo-dev nlohmann-json3-dev cmake pkg-config build-essential
   ```

2. **Clone and build**:
   ```bash
   git clone https://github.com/your-username/cv_to_osc_converter.git
   cd cv_to_osc_converter
   mkdir build && cd build
   cmake ..
   make
   ```

### Linux (Arch Linux)

1. **Install dependencies**:
   ```bash
   sudo pacman -S portaudio liblo nlohmann-json cmake pkg-config base-devel
   ```

2. **Clone and build**:
   ```bash
   git clone https://github.com/your-username/cv_to_osc_converter.git
   cd cv_to_osc_converter
   mkdir build && cd build
   cmake ..
   make
   ```

## First Run

### Quick Start
1. Connect your CV sources to your audio interface
2. Run the converter:
   ```bash
   ./cv_to_osc_converter
   ```
3. The application will auto-detect your audio interface and create a default configuration

### What Happens on First Run
```
CV to OSC Converter v1.0
=========================
Config file 'config.json' not found, using defaults
Configuration saved to config.json
Using input device: Your Audio Interface
Available channels: 8, using: 2
CV Reader initialized successfully with 2 channels
OSC sender initialized - target: 127.0.0.1:9000

Current Configuration:
  OSC Target: 127.0.0.1:9000
  Audio Device: default
  Update Rate: 100 Hz
  CV Ranges:
    Channel 1: 0V to 10V
    Channel 2: 0V to 10V

Starting CV to OSC converter...
Press Enter to stop...
```

## Configuration

### Basic Configuration

The application uses a `config.json` file for configuration:

```json
{
    "active_profile": "default",
    "profiles": {
        "default": {
            "osc_host": "127.0.0.1",
            "osc_port": "9000",
            "audio_device": "",
            "update_interval_ms": 10,
            "cv_ranges": [
                {"min": 0.0, "max": 10.0},
                {"min": 0.0, "max": 10.0},
                {"min": -5.0, "max": 5.0},
                {"min": -5.0, "max": 5.0}
            ]
        }
    }
}
```

### Configuration Profiles

You can create multiple profiles for different setups:

```json
{
    "active_profile": "studio",
    "profiles": {
        "default": {
            "osc_host": "127.0.0.1",
            "osc_port": "9000",
            "audio_device": "",
            "update_interval_ms": 10,
            "cv_ranges": [
                {"min": 0.0, "max": 10.0},
                {"min": 0.0, "max": 10.0}
            ]
        },
        "studio": {
            "osc_host": "192.168.1.100",
            "osc_port": "8000",
            "audio_device": "MOTU 8M",
            "update_interval_ms": 5,
            "cv_ranges": [
                {"min": 0.0, "max": 10.0},
                {"min": -5.0, "max": 5.0},
                {"min": 0.0, "max": 5.0},
                {"min": -10.0, "max": 10.0}
            ]
        },
        "live": {
            "osc_host": "10.0.0.50",
            "osc_port": "9001",
            "audio_device": "RME Babyface",
            "update_interval_ms": 8,
            "cv_ranges": [
                {"min": 0.0, "max": 10.0},
                {"min": 0.0, "max": 10.0},
                {"min": 0.0, "max": 10.0},
                {"min": 0.0, "max": 10.0}
            ]
        }
    }
}
```

### Profile Management

Switch between profiles at startup:
```
Available Profiles:
  default
  studio (active)
  live

Enter profile to activate or press Enter to continue: live
Profile switched to live
```

### Configuration Options Explained

| Option | Description | Default | Notes |
|--------|-------------|---------|-------|
| `osc_host` | Target OSC host | "127.0.0.1" | Use "127.0.0.1" for local, or IP for network |
| `osc_port` | Target OSC port | "9000" | Common ports: 8000, 9000, 9001 |
| `audio_device` | Audio device name | "" (auto) | Leave empty for default device |
| `update_interval_ms` | Update rate | 10 | Lower = faster updates, higher CPU |
| `cv_ranges` | Voltage ranges per channel | 0-10V | Customize per channel |

### Hot Configuration Reloading

The application automatically reloads configuration when the file changes:

1. Edit `config.json` while the application is running
2. Save the file
3. The application will automatically apply the new settings

```
Configuration changed - hot reloading...
Configuration reloaded successfully
```

## Advanced Features

### Calibration System

The application includes an auto-calibration system for precise voltage measurement:

1. **Start calibration** for a specific channel
2. **Apply known voltages** to the input
3. **Record calibration points**
4. **Finish calibration** to apply the correction

Example calibration workflow:
```cpp
// In interactive mode or via API
cvReader->startChannelCalibration(0);
cvReader->addCalibrationPoint(0, 0.0f);    // Apply 0V
cvReader->addCalibrationPoint(0, 5.0f);    // Apply 5V
cvReader->addCalibrationPoint(0, 10.0f);   // Apply 10V
auto result = cvReader->finishChannelCalibration(0);
```

### Signal Filtering

Configure signal filtering for each channel:

- **Low-pass filter**: Remove high-frequency noise
- **High-pass filter**: Remove DC offset
- **Median filter**: Remove spikes and impulse noise
- **Moving average**: Smooth rapid changes

### Performance Monitoring

The application includes real-time performance monitoring:

- **CPU usage tracking**
- **Memory usage monitoring**
- **Network latency measurement**
- **Audio buffer health**
- **OSC message statistics**

## Hardware Setup

### Eurorack Modular Setup

1. **CV Outputs**: Connect module CV outputs to audio interface inputs
2. **Voltage Ranges**: 
   - Standard Eurorack: 0V to +10V
   - Bipolar: -5V to +5V
   - Audio rate: ±5V

3. **Recommended Audio Interfaces**:
   - Expert Sleepers ES-8/ES-9
   - MOTU 8M/16A
   - RME Babyface/Fireface
   - Behringer UMC series

### Cable Connections

| CV Type | Voltage Range | Configuration | Use Case |
|---------|---------------|---------------|----------|
| Gate/Trigger | 0V to +5V | `{"min": 0.0, "max": 5.0}` | Rhythmic triggers |
| CV (Unipolar) | 0V to +10V | `{"min": 0.0, "max": 10.0}` | Standard CV |
| CV (Bipolar) | -5V to +5V | `{"min": -5.0, "max": 5.0}` | LFO, envelopes |
| Audio Rate | ±5V | `{"min": -5.0, "max": 5.0}` | Audio signals |

### Audio Interface Setup

1. **Set appropriate sample rate**: 44.1kHz or 48kHz
2. **Configure buffer size**: 64-256 samples for low latency
3. **Enable DC coupling**: If available, for accurate CV measurement
4. **Set input gains**: Adjust for proper voltage range coverage

## Troubleshooting

### Common Issues

#### 1. No Audio Device Found
**Symptoms**: Application reports "No input device available"

**Solutions**:
- Check audio interface connection
- Verify device appears in system audio settings
- Try specifying device name in config:
  ```json
  "audio_device": "Your Interface Name"
  ```
- List available devices:
  ```bash
  ./cv_to_osc_converter --list-devices
  ```

#### 2. OSC Messages Not Received
**Symptoms**: No OSC data in receiving application

**Solutions**:
- Test with OSC monitor (TouchOSC, OSCMonitor)
- Verify host/port settings
- Check firewall settings
- Try different ports (8000, 9000, 9001)
- Test local first: `"osc_host": "127.0.0.1"`

#### 3. Unstable CV Readings
**Symptoms**: Jittery or noisy CV values

**Solutions**:
- Enable signal filtering
- Check cable connections
- Verify CV source stability
- Adjust input gains on audio interface
- Use appropriate CV voltage ranges

#### 4. High CPU Usage
**Symptoms**: System becomes sluggish

**Solutions**:
- Increase update interval: `"update_interval_ms": 20`
- Reduce channel count if not needed
- Close other audio applications
- Check system performance monitor

### Debug Information

Enable verbose logging for troubleshooting:
```bash
./cv_to_osc_converter --log-level debug --verbose
```

This provides detailed information about:
- Audio device detection and setup
- OSC message transmission
- Performance metrics
- Error conditions and recovery

### System Requirements

| Component | Minimum | Recommended |
|-----------|---------|-------------|
| CPU | Dual-core 2GHz | Quad-core 2.5GHz+ |
| RAM | 4GB | 8GB+ |
| Audio Interface | 2 inputs | 8+ inputs |
| OS | macOS 10.14, Ubuntu 18.04 | Latest versions |

## Performance Tuning

### Optimizing for Low Latency

1. **Reduce update interval**:
   ```json
   "update_interval_ms": 5
   ```

2. **Use dedicated audio interface**
3. **Close unnecessary applications**
4. **Set audio interface to low buffer size**

### Optimizing for Stability

1. **Increase update interval**:
   ```json
   "update_interval_ms": 20
   ```

2. **Enable signal filtering**
3. **Use stable power supply for modular**
4. **Check cable quality**

### Network Optimization

1. **Use wired network** for remote OSC
2. **Reduce network traffic** on target network
3. **Use appropriate buffer sizes**
4. **Consider OSC message bundling** (automatic)

### Monitoring Performance

The application provides real-time performance metrics:

```
Performance Report
==================

System Statistics:
  Uptime: 127 minutes
  Total Cycles: 762000
  Total OSC Messages: 762000
  Total Dropped Samples: 0
  Total Buffer Underruns: 0

Resource Usage:
  Average CPU: 3.2%
  Peak CPU: 8.1%
  Average Memory: 12.3 MB
  Peak Memory: 15.7 MB

Performance Metrics:
  Average Latency: 2.34 ms
  Peak Latency: 8.92 ms
  Average Efficiency: 99.8%
  Minimum Efficiency: 97.2%
```

This guide should help you get started with the CV to OSC Converter and optimize it for your specific use case.
