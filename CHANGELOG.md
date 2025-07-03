# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.6.1] - 2025-07-03

### Added
- Version 1.6.1 release

### Changed
- Performance improvements and bug fixes


## [1.4.0] - 2025-07-03

### Added
- JUCE audio plugin support for DAW integration
- Device Manager for WiFi and MIDI device handling
- Advanced error handling with categorization and recovery
- macOS permissions management with automated requests
- Cross-platform compatibility improvements

### Fixed
- JUCE compatibility issues with newer versions (AudioParameterString → AudioParameterChoice)
- Windows compilation errors (ERROR macro conflicts)
- Plugin editor initialization and parameter bindings
- FontOptions compatibility for JUCE 7+
- MinGW pragma comment warnings

### Changed
- Improved plugin architecture with proper parameter handling
- Enhanced build system with better error reporting
- Updated submodule management for JUCE framework
- Better cross-platform networking support

## [1.3.0] - 2025-07-03

### Added
- Version 1.3.0 release

### Changed
- Performance improvements and bug fixes


## [1.0.0] - 2025-07-02

### Added
- Version 1.0.0 release

### Changed
- Performance improvements and bug fixes


## [Unreleased]

### Added
- Automated version management system
- GitHub Actions for CI/CD and releases
- Configuration profiles for different setups
- Hot configuration reloading
- Performance monitoring with real-time metrics
- Advanced signal filtering (low-pass, high-pass, median, exponential)
- Auto-calibration system for precise voltage measurement

### Changed
- Improved error handling with detailed categorization
- Enhanced command-line interface with version information
- Better cross-platform compatibility

## [1.0.0] - 2024-01-01

### Added
- Initial release of CV to OSC Converter
- Real-time CV to OSC conversion with sub-millisecond latency
- Configurable CV input ranges per channel (0-10V, ±5V, custom)
- OSC networking with batch message sending for efficiency
- Automatic audio device detection with up to 8 channels
- Interactive device selection with detailed specifications
- High-performance audio processing with zero-copy buffers
- RMS-based signal processing for stable CV representation
- Thread-safe architecture with mutex protection
- Command-line interface with extensive options
- Interactive mode for easy setup and monitoring
- JSON configuration with human-readable format
- Comprehensive logging with multiple severity levels
- Cross-platform support (macOS, Linux, Windows)
- Extensive testing suite with 46+ automated tests

### Technical Features
- **Performance Optimizations**: Batch OSC processing, pre-computed addresses, zero-copy buffers
- **Audio System**: PortAudio-based with auto-detection and real-time processing
- **Network**: liblo-based OSC with bundling and error recovery
- **Configuration**: JSON-based with validation and hot-reloading
- **Testing**: Comprehensive automated test suite with CI/CD integration
- **Error Handling**: Centralized system with categorization and recovery mechanisms

### Platform Support
- **macOS**: Full support with Homebrew dependencies
- **Linux**: Ubuntu/Debian/Arch support with package manager integration
- **Windows**: Basic support (future enhancement planned)

### Hardware Compatibility
- Expert Sleepers ES-8/ES-9
- MOTU 8M/16A interfaces
- RME Babyface/Fireface series
- Behringer UMC series
- Any PortAudio-compatible audio interface

### Use Cases
- **Eurorack Integration**: Connect modular synthesizers to DAWs
- **Live Performance**: Real-time CV control of software instruments
- **Studio Production**: Integrate analog and digital workflows
- **Educational**: Learn about CV/OSC protocols and audio programming
- **Research**: Audio signal processing and real-time systems
