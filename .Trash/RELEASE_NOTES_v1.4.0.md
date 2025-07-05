# CV to OSC Converter v1.4.0 Release Notes

🚀 **Major Release** - Enhanced Cross-Platform Support & Plugin Architecture

## 🎯 What's New in v1.4.0

### ✨ New Features
- **🎚️ JUCE Audio Plugin Support** - Full VST3/AU plugin integration for DAWs
- **📱 Device Manager** - Advanced WiFi and MIDI device handling with auto-discovery
- **🛡️ Enhanced Error Handling** - Comprehensive error categorization with recovery mechanisms
- **🔐 macOS Permissions Manager** - Automated permission requests with user-friendly dialogs
- **🌐 Cross-Platform Networking** - Improved Windows, macOS, and Linux compatibility

### 🔧 Technical Improvements
- **JUCE Framework Integration** - Professional audio plugin architecture
- **Smart Device Detection** - Automatic scanning and connection management
- **Memory Optimization** - Reduced memory footprint and improved performance
- **Real-time Performance** - Enhanced audio processing with lower latency

### 🐛 Critical Fixes
- ✅ **JUCE Compatibility** - Fixed compilation with newer JUCE versions (AudioParameterString → AudioParameterChoice)
- ✅ **Windows Builds** - Resolved ERROR macro conflicts and MinGW warnings  
- ✅ **Plugin Editor** - Fixed parameter bindings and UI initialization
- ✅ **Font Rendering** - Updated to FontOptions for JUCE 7+ compatibility
- ✅ **Cross-Platform Builds** - Fixed pragma comment warnings for different compilers

## 📦 Release Assets

### macOS
- **CV-to-OSC-Converter-v1.4.0.dmg** - Complete macOS application bundle
  - Native .app with code signing
  - Terminal command shortcuts
  - Comprehensive documentation

### Audio Plugin Formats
- **VST3 Plugin** - For DAW integration (Ableton Live, Logic Pro, etc.)
- **Audio Unit (AU)** - Native macOS plugin format
- **Standalone Application** - Independent audio application

## 🛠️ System Requirements

### macOS
- macOS 10.15 (Catalina) or later
- Intel x64 or Apple Silicon (M1/M2)
- Audio interface with CV outputs (recommended)

### Windows
- Windows 10 64-bit or later
- ASIO-compatible audio interface (recommended)

### Linux
- Ubuntu 20.04+ / Debian 11+ / Arch Linux
- ALSA or PulseAudio support
- GCC 9+ or Clang 10+

## 🚀 Quick Start

### For End Users
1. Download the appropriate release for your platform
2. Install/extract the application
3. Run permission setup: `./cv_to_osc_converter --request-permissions`
4. Launch in interactive mode: `./cv_to_osc_converter -i`

### For Developers
```bash
git clone https://github.com/pr-sk/CV-to-OSC-converter.git
cd CV-to-OSC-converter
git submodule update --init --recursive
make
```

## 🔄 Migration from v1.3.x

### Configuration
- Existing config.json files remain compatible
- New device management settings added automatically
- Profile system enhanced with additional options

### API Changes
- Plugin parameter system updated (affects custom integrations)
- Error handling callbacks modified for better categorization
- OSC message format remains unchanged

## 🐛 Known Issues

- Plugin builds require additional PortAudio configuration on some systems
- Some DAWs may require plugin rescanning after installation
- WiFi device discovery may be limited by network security settings

## 🔧 Technical Details

### Build Information
- **Version**: 1.4.0
- **Build Date**: 2025-07-03
- **Git Commit**: 5a2cb0c
- **JUCE Version**: 7.0.9
- **Compiler**: Apple LLVM 17.0.0 (clang-1700.0.13.5)

### Dependencies
- JUCE Framework 7.0.9+
- PortAudio v19
- liblo (OSC library)
- nlohmann/json 3.12.0+

## 📚 Documentation

- **User Manual**: [docs/USER_GUIDE.md](docs/USER_GUIDE.md)
- **Developer Guide**: [docs/DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md)
- **API Reference**: [docs/API_REFERENCE.md](docs/API_REFERENCE.md)
- **Plugin Development**: [docs/PLUGIN_DEVELOPMENT.md](docs/PLUGIN_DEVELOPMENT.md)

## 🤝 Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Reporting Issues
- Use GitHub Issues for bug reports
- Include system information and reproduction steps
- Check existing issues before creating new ones

## 📄 License

This project is licensed under the MIT License - see [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- JUCE Team for the excellent audio framework
- PortAudio community for cross-platform audio support
- liblo developers for OSC implementation
- All contributors and testers who made this release possible

---

**Download Links:**
- [macOS DMG](../../releases/download/v1.4.0/CV-to-OSC-Converter-v1.4.0.dmg)
- [Windows Installer](../../releases/download/v1.4.0/CV-to-OSC-Converter-v1.4.0-Windows.exe)
- [Linux AppImage](../../releases/download/v1.4.0/CV-to-OSC-Converter-v1.4.0-Linux.AppImage)
- [Source Code](../../archive/refs/tags/v1.4.0.tar.gz)

**Support:** For questions and support, please visit our [Discussions](../../discussions) page.
