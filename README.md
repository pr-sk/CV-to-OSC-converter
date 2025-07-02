# CV to OSC Converter

[![Build Status](https://github.com/pr-sk/cv_to_osc_converter/workflows/CI/badge.svg)](https://github.com/pr-sk/cv_to_osc_converter/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux%20%7C%20Windows-lightgrey)]()

High-performance C++ application that converts CV signals to OSC messages in real-time, ideal for integrating modular synthesizers with digital audio workstations and live performance setups.

## 🚀 Quick Start

### macOS
```bash
# Install dependencies
brew install portaudio liblo nlohmann-json cmake pkg-config

# Clone and build
git clone https://github.com/pr-sk/cv_to_osc_converter.git
cd cv_to_osc_converter
mkdir build && cd build
cmake ..
make

# Run
./cv_to_osc_converter
```

### Windows (MSYS2)
```bash
# Install MSYS2 from https://www.msys2.org/
# Open MSYS2 MinGW 64-bit shell and install dependencies
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-pkg-config
pacman -S mingw-w64-x86_64-portaudio mingw-w64-x86_64-liblo mingw-w64-x86_64-nlohmann-json

# Clone and build
git clone https://github.com/pr-sk/cv_to_osc_converter.git
cd cv_to_osc_converter
mkdir build && cd build
cmake -G "MSYS Makefiles" ..
make

# Run
./cv_to_osc_converter.exe
```

### Linux (Ubuntu/Debian)
```bash
# Install dependencies
sudo apt-get update
sudo apt-get install cmake pkg-config build-essential
# Try installing PortAudio (one of these should work)
sudo apt-get install portaudio19-dev || sudo apt-get install libportaudio2-dev
sudo apt-get install liblo-dev
sudo apt-get install nlohmann-json3-dev || sudo apt-get install nlohmann-json-dev

# Clone and build
git clone https://github.com/pr-sk/cv_to_osc_converter.git
cd cv_to_osc_converter
mkdir build && cd build
cmake ..
make

# Run
./cv_to_osc_converter
```


## ✨ Features

- **Real-time CV to OSC conversion** with low latency
- **Multi-channel support** with configurable voltage ranges
- **Interactive mode** for easy setup and monitoring
- **Hot configuration reloading** without restart
- **Cross-platform support** (macOS, Linux, Windows)
- **Automatic audio device detection**
- **JSON configuration** with human-readable format

## Dependencies

### macOS (via Homebrew)
```bash
brew install portaudio liblo nlohmann-json cmake pkg-config
# If you encounter compilation issues, you may also need:
brew install gcc
```

### Windows

**Option 1: Using MSYS2 (Recommended)**
```bash
# Install MSYS2 from https://www.msys2.org/
# Open MSYS2 MinGW 64-bit shell and run:
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-pkg-config
pacman -S mingw-w64-x86_64-portaudio
pacman -S mingw-w64-x86_64-liblo
pacman -S mingw-w64-x86_64-nlohmann-json
```

**Option 2: Using vcpkg**
```bash
# Install vcpkg from https://github.com/Microsoft/vcpkg
# Then install dependencies:
vcpkg install portaudio
vcpkg install liblo
vcpkg install nlohmann-json
```

**Option 3: Manual Installation**
- Download and compile PortAudio from http://www.portaudio.com/
- Download and compile liblo from http://liblo.sourceforge.net/
- Download nlohmann/json headers from https://github.com/nlohmann/json

### Ubuntu/Debian
```bash
# Install basic build tools first
sudo apt-get update
sudo apt-get install cmake pkg-config build-essential

# Try to install PortAudio (try these in order)
sudo apt-get install portaudio19-dev || \
sudo apt-get install libportaudio2-dev || \
sudo apt-get install libportaudio-dev

# Install other dependencies
sudo apt-get install liblo-dev
sudo apt-get install nlohmann-json3-dev || sudo apt-get install nlohmann-json-dev

# Alternative one-liner (tries common package names):
# sudo apt-get install portaudio19-dev liblo-dev nlohmann-json3-dev cmake pkg-config build-essential
```

### Arch Linux
```bash
sudo pacman -S portaudio liblo nlohmann-json cmake pkg-config
```

## Building

### macOS and Linux

1. Clone or download the source code
2. Create a build directory and navigate to it:
```bash
mkdir build && cd build
```

3. Configure with CMake:
```bash
cmake ..
```

4. Build the application:
```bash
make
```

### Windows

#### Using MSYS2 (Recommended)
1. Open MSYS2 MinGW 64-bit shell
2. Navigate to the project directory
3. Create build directory and configure:
```bash
mkdir build && cd build
cmake -G "MSYS Makefiles" ..
make
```

#### Using Visual Studio
1. Open Visual Studio Command Prompt or PowerShell
2. Navigate to the project directory
3. Create build directory and configure:
```bash
mkdir build
cd build
cmake -G "Visual Studio 16 2019" ..
cmake --build . --config Release
```

#### Using vcpkg
If using vcpkg, add the toolchain file:
```bash
cmake -DCMAKE_TOOLCHAIN_FILE=[vcpkg_root]/scripts/buildsystems/vcpkg.cmake ..
```

### Windows Build Notes

- **MSYS2**: Recommended for most users. Provides a Unix-like environment with MinGW-w64 compiler
- **Visual Studio**: Use Visual Studio 2019 or later with C++ development tools
- **vcpkg**: Good for managing dependencies in Visual Studio projects
- Ensure all dependencies are compiled with the same compiler toolchain
- If using MSYS2, always use the MinGW 64-bit shell for consistency
- Windows Defender may flag the executable - add build directory to exclusions

### macOS Build Notes

- The project is configured to build for the current architecture only (arm64 on Apple Silicon, x86_64 on Intel Macs)
- If you encounter linking errors related to architecture mismatches, ensure all dependencies are installed for your system's architecture
- GCC may be required for some compilation scenarios; install with `brew install gcc` if needed

### Linux Build Notes

- Most distributions include the required dependencies in their package managers
- For older Ubuntu versions, some package names may differ (see Dependencies section)
- Arch Linux users should install from the official repositories for best compatibility
- If building from source, ensure development headers are installed

### Troubleshooting Build Issues

**nlohmann/json header not found:**
- Ensure nlohmann-json is installed: `brew install nlohmann-json`
- The CMakeLists.txt includes the correct path automatically

**Architecture linking errors:**
- Clean and rebuild: `rm -rf build && mkdir build && cd build && cmake .. && make`
- Ensure all dependencies match your system architecture

**liblo function errors:**
- The build process handles compatibility issues with different liblo versions automatically

## Performance Optimizations

The application has been optimized for maximum performance:

- **Batch OSC Processing**: Messages are sent in bundles for better network efficiency
- **Pre-computed OSC Addresses**: String concatenation is avoided in the main loop
- **Zero-copy Buffer Management**: Reusable buffers minimize memory allocations
- **RMS Signal Processing**: Better CV signal representation compared to simple averaging
- **Thread-safe Design**: Mutex-protected access to shared audio data
- **Compiler Optimizations**: Release build with `-O3` and platform-specific optimizations
- **Precise Timing Control**: High-resolution timing for consistent update rates

## Configuration

The application uses a `config.json` file for configuration. If the file doesn't exist, it will be created with default values on first run.

### Configuration Options

- `osc_host`: Target OSC host (default: "127.0.0.1")
- `osc_port`: Target OSC port (default: "9000")
- `audio_device`: Audio device name (empty = use default device)
- `update_interval_ms`: Update rate in milliseconds (default: 10ms = 100Hz)
- `cv_ranges`: Array of CV input ranges for each channel

### Example Configuration

```json
{
    "osc_host": "127.0.0.1",
    "osc_port": "9000",
    "audio_device": "",
    "update_interval_ms": 10,
    "cv_ranges": [
        {
            "min": 0.0,
            "max": 10.0,
            "comment": "Standard Eurorack CV (0-10V)"
        },
        {
            "min": -5.0,
            "max": 5.0,
            "comment": "Bipolar CV (-5V to +5V)"
        },
        {
            "min": -1.0,
            "max": 1.0,
            "comment": "Audio rate signals"
        }
    ]
}
```

### Configuration Tips

- **update_interval_ms**: Lower values = higher update rate (10ms = 100Hz is recommended)
- **cv_ranges**: Configure each channel's voltage range for accurate normalization
- **audio_device**: Leave empty to use default, or specify device name for specific interface
- **Network latency**: For local use, 127.0.0.1 is optimal. For network use, specify target IP

## Usage

1. **Connect your CV sources** to your audio interface inputs
2. **Run the converter** (it will auto-create config.json on first run):
```bash
./cv_to_osc_converter
```

3. **Configure if needed** by editing the generated `config.json` file
4. **Restart the application** to apply new settings
5. The application will:
   - Auto-detect available audio channels
   - Display current configuration
   - Start real-time CV to OSC conversion
6. **Press Enter** to stop the converter

## OPTIONS

1. **Run HELP
```bash
./cv_to_osc_converter -h                       

Convert Control Voltage signals to Open Sound Control messages

Usage:
  cv_to_osc_converter [OPTIONS]

Options:
  -h, --help              Show this help message
  -v, --version           Show version information
  -i, --interactive       Run in interactive mode
  -l, --list-devices      List available audio devices
  -d, --daemon            Run as daemon (background mode)
  -c, --config FILE       Use specific config file (default: config.json)
  --verbose               Enable verbose output
  -q, --quiet             Suppress non-essential output

Configuration Overrides:
  --osc-host HOST         Override OSC target host
  --osc-port PORT         Override OSC target port
  --audio-device NAME     Override audio device
  --update-interval MS    Override update interval (milliseconds)
  --log-level LEVEL       Set log level (debug, info, warn, error)

Examples:
  ./cv_to_osc_converter                     # Run with default settings
  ./cv_to_osc_converter -i                  # Run in interactive mode
  ./cv_to_osc_converter -l                  # List audio devices
  ./cv_to_osc_converter --osc-host 192.168.1.100 --osc-port 8000
  ./cv_to_osc_converter -c my_config.json   # Use custom config file
  ./cv_to_osc_converter -d --quiet          # Run as quiet daemon
```

### First Run Example
```
CV to OSC Converter v1.0
=========================
Config file 'config.json' not found, using defaults
Configuration saved to config.json
Using input device: Your Audio Interface
Available channels: 8, using: 8
CV Reader initialized successfully with 8 channels
OSC sender initialized - target: 127.0.0.1:9000

Current Configuration:
  OSC Target: 127.0.0.1:9000
  Audio Device: default
  Update Rate: 100 Hz
  CV Ranges:
    Channel 1: 0V to 10V
    Channel 2: 0V to 10V
    ...

Press Enter to stop...
```

## OSC Message Format

The converter sends OSC messages with the following format:

- **Address**: `/cv/channel/N` (where N is the channel number, starting from 1)
- **Type**: Float (32-bit)
- **Value**: Normalized value between 0.0 and 1.0

### Example OSC Messages

```
/cv/channel/1 0.5    # Channel 1 at 50% of its configured range
/cv/channel/2 0.0    # Channel 2 at minimum value
/cv/channel/3 1.0    # Channel 3 at maximum value
```

## Hardware Setup

### Typical Eurorack Setup

1. Use CV outputs from your Eurorack modules
2. Connect to your audio interface inputs via appropriate cables
3. Configure CV ranges in the config file:
   - **Unipolar CV** (0-10V): Set min=0.0, max=10.0
   - **Bipolar CV** (-5V to +5V): Set min=-5.0, max=5.0
   - **Audio rate** (±5V): Set min=-5.0, max=5.0

### Audio Interface Considerations

- Use an audio interface with sufficient input channels
- Ensure the interface can handle the voltage levels from your CV sources
- Some interfaces may require attenuation for high CV voltages
- DC coupling is preferred for accurate CV representation


## Testing

The CV to OSC converter includes a comprehensive automated testing suite:

### Running Tests

```bash
# Quick test run
./run_tests.sh

# Manual test execution
cd build_tests
g++ -std=c++17 -O2 -I/usr/local/Cellar/nlohmann-json/3.12.0/include ../tests/simple_test.cpp -o simple_test
./simple_test
```

### Test Coverage

- ✅ **46 automated tests** with 100% pass rate
- ✅ **Configuration management** - defaults, file I/O, edge cases
- ✅ **JSON processing** - valid and malformed data handling
- ✅ **CV normalization** - voltage range conversion logic
- ✅ **Performance validation** - timing and efficiency checks
- ✅ **Error handling** - graceful failure modes

### Continuous Integration

Automated testing runs on:
- **macOS** - Full integration testing with audio system
- **Ubuntu** - Cross-platform compatibility validation
- **Code Quality** - Static analysis and warning-free compilation

See `TESTING.md` for detailed documentation.

## Performance Monitoring

The optimized version provides significantly better performance:

- **Reduced CPU usage** through batch processing and optimized algorithms
- **Lower memory allocation** with pre-allocated buffers
- **Improved timing accuracy** with high-resolution timing control
- **Better signal quality** with RMS-based processing
- **Network efficiency** through OSC message bundling

## Troubleshooting

### No Audio Device Found
- Check that your audio interface is connected and recognized by the system
- Try specifying the device name in the config file
- Run `system_profiler SPAudioDataType` (macOS) to see available devices
- Check Audio MIDI Setup (macOS) for device status

### OSC Messages Not Received
- Verify the OSC host and port in the configuration
- Check firewall settings (macOS: System Preferences > Security & Privacy > Firewall)
- Test with an OSC monitor application (like OSCMonitor or TouchOSC)
- Try different ports (9000, 8000, 7000)

### Performance Issues
- **High CPU usage**: Increase `update_interval_ms` (try 20ms for 50Hz)
- **Audio dropouts**: Check audio interface buffer settings
- **Network congestion**: Reduce update rate or use local network
- **Memory issues**: The app now uses optimized buffers to minimize allocations

### Channel Detection Issues
- The app auto-detects available channels but starts conservatively
- Check your audio interface specifications
- Some interfaces may report more channels than physically available
- Configure `audio_device` name for specific interface selection

## License

This project is open source. Feel free to modify and distribute according to your needs.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.
