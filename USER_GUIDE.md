# CV to OSC Converter - User Guide

This comprehensive guide will walk you through setting up and using the CV to OSC Converter for your audio production workflow. The application provides professional-grade CV to OSC conversion with advanced features for musicians, sound designers, and developers.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Installation](#installation)
3. [Basic Usage](#basic-usage)
4. [Interactive Mode](#interactive-mode)
5. [Configuration](#configuration)
6. [Advanced Features](#advanced-features)
7. [Hardware Setup](#hardware-setup)
8. [Security & OSC Features](#security--osc-features)
9. [Performance Monitoring](#performance-monitoring)
10. [Troubleshooting](#troubleshooting)
11. [Performance Tuning](#performance-tuning)

## Quick Start

To get up and running quickly:

1. Install dependencies
2. Build the application
3. Run with default settings or use interactive mode
4. Configure for your specific setup

### macOS Quick Start

```bash
# Install dependencies
brew install portaudio liblo nlohmann-json cmake pkg-config

# Clone and build
git clone https://github.com/your-username/cv_to_osc_converter.git
cd cv_to_osc_converter
mkdir build && cd build
cmake ..
make

# Run interactively (recommended for first-time users)
./cv_to_osc_converter --interactive

# Or run with defaults
./cv_to_osc_converter
```

### Ubuntu Quick Start

```bash
# Install dependencies
sudo apt-get install libportaudio2-dev liblo-dev nlohmann-json3-dev cmake pkg-config build-essential

# Clone and build
git clone https://github.com/your-username/cv_to_osc_converter.git
cd cv_to_osc_converter
mkdir build && cd build
cmake ..
make

# Run interactively
./cv_to_osc_converter --interactive
```

## Installation

### Prerequisites

The CV to OSC Converter requires the following dependencies:

- **PortAudio**: For audio interface access
- **liblo**: For OSC message handling
- **nlohmann/json**: For configuration file parsing
- **CMake**: For build system
- **pkg-config**: For dependency management

### Platform-Specific Installation

#### macOS (Homebrew)
```bash
brew install portaudio liblo nlohmann-json cmake pkg-config
```

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install libportaudio2-dev liblo-dev nlohmann-json3-dev cmake pkg-config build-essential
```

#### Arch Linux
```bash
sudo pacman -S portaudio liblo nlohmann-json cmake pkg-config
```

### Building from Source

1. Clone the repository:
```bash
git clone https://github.com/your-username/cv_to_osc_converter.git
cd cv_to_osc_converter
```

2. Create build directory:
```bash
mkdir build && cd build
```

3. Configure with CMake:
```bash
cmake ..
```

4. Build:
```bash
make
```

### Testing the Build

Run the test suite to verify everything is working:
```bash
# From project root
./run_tests.sh
```

You should see:
```
✅ All tests passed! The CV to OSC converter is working correctly.
```

## Basic Usage

### Command Line Options

The application provides extensive command-line options:

```bash
# Show help
./cv_to_osc_converter --help

# Show version information
./cv_to_osc_converter --version

# List available audio devices
./cv_to_osc_converter --list-devices

# Run in interactive mode
./cv_to_osc_converter --interactive

# Run as daemon (background)
./cv_to_osc_converter --daemon --quiet

# Use custom config file
./cv_to_osc_converter --config my_config.json

# Override settings temporarily
./cv_to_osc_converter --osc-host 192.168.1.100 --osc-port 8000

# Enable debug logging
./cv_to_osc_converter --log-level debug --verbose
```

### First Run

On first run, the application will:
1. Create a default `config.json` file
2. Auto-detect your audio interface
3. Display current configuration
4. Start conversion with default settings

Example first run output:
```
CV to OSC Converter v1.0.0-dev
==============================
Config file not found, creating default configuration
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

## Interactive Mode

Interactive mode provides a user-friendly menu system for configuration and monitoring:

```bash
./cv_to_osc_converter --interactive
```

### Main Menu

```
===================================================
   CV to OSC Converter - Interactive Mode
===================================================

Main Menu
----------------------------------------
1. Start CV to OSC Converter
2. Configuration Settings
3. Audio Device Selection
4. Monitoring & Diagnostics
5. Run Tests
6. Exit
```

### Configuration Menu

Access comprehensive configuration options:

- **OSC Settings**: Change host, port, and message format
- **Audio Device**: Select specific interface or use default
- **Update Rate**: Adjust conversion frequency (1-1000 Hz)
- **CV Ranges**: Configure voltage ranges per channel
- **Save Configuration**: Persist settings to file

### Audio Device Menu

Comprehensive audio device management:

1. **List All Devices**: Show every audio device on system
2. **List Input Devices**: Show only devices with input capabilities
3. **Device Details**: Get detailed specifications for any device
4. **Test Device**: Verify device functionality and format support
5. **Search Devices**: Find devices by name
6. **Refresh List**: Update device list (detects hot-plugged devices)
7. **Status Report**: System-wide audio device health check

### Monitoring Menu

Real-time monitoring and diagnostics:

- **Live CV Monitor**: View current CV values in real-time
- **OSC Connection Test**: Verify OSC message delivery
- **Audio Device Status**: Check device health and performance
- **Performance Metrics**: View system performance statistics

## Configuration

### Configuration Profiles

The application supports multiple configuration profiles for different setups:

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

#### Manual Calibration Workflow
1. **Start calibration** for a specific channel
2. **Apply known voltages** to the input
3. **Record calibration points**
4. **Finish calibration** to apply the correction

#### Calibration Presets
- **Eurorack Configuration**: 0-10V with high precision
- **Bipolar Configuration**: -5V to +5V range
- **Audio Rate Configuration**: ±5V for audio signals

### Signal Filtering

Configure advanced signal filtering for each channel:

- **Low-pass filter**: Remove high-frequency noise
- **High-pass filter**: Remove DC offset and low-frequency drift
- **Median filter**: Remove spikes and impulse noise
- **Moving average**: Smooth rapid changes
- **Exponential filter**: Gentle smoothing with configurable response

#### Filter Factory Presets
- **CV Filter**: Optimized for control voltage signals
- **Audio Filter**: Optimized for audio-rate signals
- **Smoothing Filter**: Gentle signal smoothing
- **Noise Reduction**: Aggressive noise filtering

### OSC Message Format

The converter sends OSC messages with the following format:

- **Address**: `/cv/channel/N` (where N is the channel number, starting from 1)
- **Type**: Float (32-bit)
- **Value**: Normalized value between 0.0 and 1.0

Example OSC Messages:
```
/cv/channel/1 0.5    # Channel 1 at 50% of its configured range
/cv/channel/2 0.0    # Channel 2 at minimum value
/cv/channel/3 1.0    # Channel 3 at maximum value
```

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

## Security & OSC Features

### OSC Security Configuration

The application includes comprehensive OSC security features:

```cpp
// Security configuration
OSCSecurity::SecurityConfig secConfig;
secConfig.enableValidation = true;
secConfig.enableSanitization = true;
secConfig.enableRateLimiting = true;
secConfig.maxMessagesPerSecond = 1000;
secConfig.maxFloatValue = 1000000.0f;
secConfig.minFloatValue = -1000000.0f;
```

### Security Features

- **Address Validation**: Ensures OSC addresses follow proper format
- **Value Sanitization**: Clamps values to safe ranges
- **Rate Limiting**: Prevents OSC message flooding
- **Host Whitelisting**: Restrict OSC targets to approved hosts
- **Message Validation**: Comprehensive message format checking

### OSC Message Security

- **Float Range Validation**: Prevents overflow/underflow
- **String Length Limits**: Prevents buffer overflow attacks
- **Blob Size Limits**: Controls memory usage
- **Pattern Matching**: Validates address patterns

## Performance Monitoring

### Real-Time Metrics

The application provides comprehensive performance monitoring:

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

### Performance Alerts

Automatic alerting for performance issues:

- **CPU Usage**: Warnings at 70%, critical at 90%
- **Memory Usage**: Warnings at 80MB, critical at 150MB
- **Latency**: Warnings at 20ms, critical at 50ms
- **Efficiency**: Warnings below 80%, critical below 60%

### Monitoring Configuration

Configure monitoring behavior:

```cpp
PerformanceMonitor::MonitorConfig config;
config.updateInterval = std::chrono::milliseconds(1000);
config.enableDetailedMetrics = true;
config.enableAlerts = true;
config.logToFile = true;
```

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

### Docker Deployment

For containerized deployment:

```bash
# Build Docker image
docker build -t cv-to-osc .

# Run with basic configuration
docker run -d --name cv-to-osc \
  --network host \
  --device /dev/snd:/dev/snd \
  cv-to-osc

# Run with custom configuration
docker run -d --name cv-to-osc \
  --network host \
  --device /dev/snd:/dev/snd \
  -v $(pwd)/custom-config.json:/app/config.json:ro \
  cv-to-osc
```

### Performance Profiles

Use predefined performance profiles:

```cpp
// High performance (low latency)
auto config = MonitorConfigFactory::createHighPerformanceConfig();

// Production (stable, logged)
auto config = MonitorConfigFactory::createProductionConfig();

// Debug (detailed monitoring)
auto config = MonitorConfigFactory::createDebugConfig();
```

This guide provides comprehensive coverage of all the CV to OSC Converter's features and capabilities. For additional support, check the project's issue tracker or documentation.
