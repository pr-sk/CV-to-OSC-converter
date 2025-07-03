# CV to OSC Converter v1.1.0 - GUI Enhancements Release

**Release Date**: July 3, 2025  
**Version**: 1.1.0  
**Build Type**: Release

## 🎯 Major Features

### 🔄 Bidirectional CV/OSC Conversion
- **NEW**: OSC to CV conversion support
- Receive OSC messages and convert them to CV output
- Configurable listening port for incoming OSC data
- Real-time OSC to CV signal conversion
- Seamless integration with existing CV to OSC functionality

### 🎛️ Enhanced GUI Interface
- **NEW**: CVWriter class for audio output capabilities
- **NEW**: Command system with undo/redo support
- **NEW**: Drag and drop channel reordering
- **IMPROVED**: Real-time CV signal visualization
- **IMPROVED**: Better error handling and user feedback

### 🔧 Code Architecture Improvements
- Modular CVWriter implementation for future expansion
- Enhanced command pattern for user actions
- Better separation of concerns between components
- Improved memory management and performance

## 🆕 New Features

### Audio Engine
- **CVWriter class**: New audio output engine for OSC to CV conversion
- **Bidirectional support**: Handle both CV → OSC and OSC → CV workflows
- **Voltage range control**: Configurable output voltage ranges
- **Multi-channel support**: Up to 32 simultaneous CV output channels

### User Interface
- **Drag & Drop**: Reorder channels by dragging in the interface
- **Command System**: Full undo/redo support for all user actions
- **OSC Listening**: Configure and monitor incoming OSC connections
- **Enhanced Status**: Better connection and performance indicators

### Network Protocol
- **OSC Input**: Listen for incoming OSC messages on configurable port
- **Format Support**: Standard /cv/channel/N addressing scheme
- **Normalized Values**: 0.0-1.0 OSC values mapped to CV voltage ranges
- **Real-time Processing**: Low-latency OSC to CV conversion

## 🔨 Technical Improvements

### Performance
- **Optimized Build**: Improved compilation flags for better performance
- **Memory Usage**: Reduced memory footprint and better resource management
- **Audio Latency**: Minimized latency in audio processing chain
- **Thread Safety**: Enhanced thread safety for real-time operations

### Code Quality
- **Error Handling**: Comprehensive error checking and user feedback
- **Debug Support**: Enhanced debugging capabilities for development
- **Documentation**: Improved code documentation and inline comments
- **Testing**: Better test coverage for new features

### Dependencies
- **GLFW 3.4**: Updated windowing library for better compatibility
- **GLEW 2.2**: Updated OpenGL extension handling
- **CMake Build**: Enhanced build system with better dependency management
- **macOS Support**: Improved macOS compatibility and performance

## 🐛 Bug Fixes

- Fixed compilation errors with missing command classes
- Resolved duplicate command definitions between files
- Fixed CMakeLists.txt to include new CVWriter components
- Improved GUI thread safety and stability
- Fixed memory leaks in audio processing components

## 🔧 Developer Notes

### Build System
- Added CVWriter.cpp and CVWriter.h to build configuration
- Updated CMakeLists.txt for both CLI and GUI executables
- Enhanced pkg-config support for macOS dependencies
- Improved cross-platform compilation support

### API Changes
- New CVWriter class with voltage range control methods
- Enhanced command interface for undo/redo operations
- Updated OSCReceiver with callback support
- Improved error reporting throughout the codebase

## 📦 Installation

### macOS
```bash
# Install dependencies
./install_gui_deps.sh

# Build the application
make clean && make

# Or build manually
mkdir -p build && cd build
export PKG_CONFIG_PATH="/usr/local/Cellar/glfw/3.4/lib/pkgconfig:/usr/local/Cellar/glew/2.2.0_1/lib/pkgconfig:$PKG_CONFIG_PATH"
cmake .. -DBUILD_GUI=ON
make -j8
```

### Usage
```bash
# GUI version
./build/cv_to_osc_converter_gui

# CLI version
./build/cv_to_osc_converter
```

## 🔮 What's Next

### Planned for v1.2.0
- **File Dialog System**: Native file dialogs for configuration management
- **Audio Device Management**: Dynamic device detection and switching
- **Advanced Performance Metrics**: Detailed system performance monitoring
- **MIDI Integration**: MIDI input/output support for enhanced control

### Future Roadmap
- **Plugin Architecture**: Support for third-party extensions
- **Web Interface**: Browser-based remote control interface
- **Advanced Automation**: Scripting support with Lua/Python
- **Cloud Integration**: Remote configuration and monitoring

## 📝 Breaking Changes

None in this release. All existing configurations and workflows remain fully compatible.

## 🙏 Acknowledgments

This release includes improvements to the GUI architecture, enhanced audio processing capabilities, and better user experience. Special thanks to the community for feedback and testing.

## 📞 Support

- **Documentation**: See USER_GUIDE.md for detailed usage instructions
- **Issues**: Report issues on GitHub
- **Discussions**: Join community discussions for feature requests

---

**Full Changelog**: v1.0.0...v1.1.0
