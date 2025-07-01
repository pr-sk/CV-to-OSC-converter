# CV to OSC Converter

A C++ application that converts Control Voltage (CV) signals from audio interfaces to Open Sound Control (OSC) messages. This is useful for interfacing analog modular synthesizers with digital audio workstations or other OSC-compatible software.

## Features

- **Real-time CV to OSC conversion** with optimized performance
- **Configurable CV input ranges** per channel for precise voltage mapping
- **Configurable OSC output target** (host and port)
- **Automatic channel detection** - adapts to available audio interface channels
- **Batch OSC message sending** for improved network efficiency
- **Thread-safe audio processing** with mutex protection
- **RMS-based signal processing** for better CV signal representation
- **JSON-based configuration** with automatic config file generation
- **Compiler optimizations** for maximum performance
- **Cross-platform support** (macOS, Linux, Windows)

- **--list-devices - List available audio devices
- **--list-devices --verbose - Detailed device information

- **Interactive device menu with:
•  List all devices or input-only devices
•  Show detailed device specifications
•  Test device availability and format support
•  Search devices by name
•  Real-time device list refresh
•  Device status reports
•  Device compatibility testing for different channel counts and sample rates
•  Real-time availability detection
•  Automatic default device identification

## TODO

1. ✅ User Interface:
- **Implement a graphical interface or command-line options for improved usability.
2. ✅ Audio Device Management:
- **List and select available audio devices interactively.
- **Add real-time monitoring of device connectivity.
3. Advanced Error Handling:
- **More descriptive error messages and guidance.
- **Automatic recovery or retries for non-critical failures.
4. Performance Optimization:
- **Profile the application for bottlenecks.
- **Utilize multi-threading more effectively.
5. OSC Enhancements:
- **Allow custom OSC message formatting.
- **Support additional OSC types besides float (e.g., integers, strings).
6. Extended Configuration:
- **In-app configuration editing.
- **Support for multiple configuration profiles.
7. Extensive Testing:
- **Add more nuanced test cases for edge scenarios.
- **Include performance and stress testing.
8. Cross-Platform Support:
- **Ensure compatibility with Windows.
- **Develop a Docker image for easy deployment.
9. Documentation:
- **Add detailed in-code comments and docstrings.
- **Improve user guide and FAQ section.
10. Security Features:
- **Validate and sanitize OSC input/output.
- **Consider encryption for sensitive OSC messages.

## Dependencies

### macOS (via Homebrew)
```bash
brew install portaudio liblo nlohmann-json cmake pkg-config
# If you encounter compilation issues, you may also need:
brew install gcc
```

### Ubuntu/Debian
```bash
sudo apt-get install libportaudio2-dev liblo-dev nlohmann-json3-dev cmake pkg-config build-essential
```

### Arch Linux
```bash
sudo pacman -S portaudio liblo nlohmann-json cmake pkg-config
```

## Building

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

### macOS Build Notes

- The project is configured to build for the current architecture only (arm64 on Apple Silicon, x86_64 on Intel Macs)
- If you encounter linking errors related to architecture mismatches, ensure all dependencies are installed for your system's architecture
- GCC may be required for some compilation scenarios; install with `brew install gcc` if needed

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
