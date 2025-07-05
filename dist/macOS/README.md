# CV to OSC Converter

🎛️ **Professional CV ↔ OSC converter** with real-time bidirectional signal processing and advanced GUI.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux%20%7C%20Windows-lightgrey)](https://github.com/prubtsov/cv_to_osc_converter)

## ✨ Features

- **Real-time CV to OSC conversion** with sub-10ms latency
- **Bidirectional OSC ↔ CV** for complete signal integration  
- **Multi-channel support** with individual voltage range configuration
- **Professional GUI** with real-time visualization and waveform plotting
- **Cross-platform** support (macOS, Linux, Windows)

## 🚀 Quick Start

### macOS (Recommended)
```bash
# Install dependencies
brew install portaudio liblo nlohmann-json cmake pkg-config

# Build
git clone https://github.com/prubtsov/cv_to_osc_converter.git
cd cv_to_osc_converter
mkdir build && cd build
cmake .. -DBUILD_GUI=ON
make -j$(sysctl -n hw.ncpu)

# Run
./cv_to_osc_converter_gui
```

### Linux (Ubuntu/Debian)
```bash
# Install dependencies
sudo apt-get install portaudio19-dev liblo-dev nlohmann-json3-dev cmake pkg-config build-essential

# Build  
git clone https://github.com/prubtsov/cv_to_osc_converter.git
cd cv_to_osc_converter
mkdir build && cd build
cmake .. -DBUILD_GUI=ON
make

# Run
./cv_to_osc_converter_gui
```

## 📁 Project Structure

```
cv_to_osc_converter/
├── src/                    # Source code (organized by functionality)
│   ├── core/              # Core system components
│   ├── audio/             # Audio processing
│   ├── osc/               # OSC communication  
│   ├── gui/               # User interface
│   ├── platform/          # Platform-specific code
│   ├── utils/             # Utility functions
│   └── config/            # Configuration headers
├── docs/                  # Documentation
├── scripts/               # Build and utility scripts
├── tests/                 # Test files
├── config/               # Configuration files
├── assets/               # Resources (icons, images)
├── plugins/              # Plugin source code
└── build/                # Build output
```

## 📖 Documentation

| Document | Description |
|----------|-------------|
| [Full README](docs/README.md) | Complete documentation with all features |
| [GUI Guide](docs/GUI_GUIDE.md) | GUI-specific instructions and features |
| [Build Guide](docs/TESTING.md) | Detailed build instructions for all platforms |
| [User Guide](docs/USER_GUIDE.md) | End-user documentation |
| [Changelog](docs/CHANGELOG.md) | Version history and changes |

## 🔧 Development

- **Language**: C++17
- **GUI**: Dear ImGui + OpenGL
- **Audio**: PortAudio
- **OSC**: liblo
- **Build**: CMake + Make
- **Platform**: Cross-platform (macOS, Linux, Windows)

## 🎯 Use Cases

- **Eurorack Integration**: Convert CV signals to control digital instruments
- **Live Performance**: Real-time signal processing and routing
- **Studio Production**: Bridge analog and digital workflows
- **Educational**: Learn CV/OSC protocols and real-time audio

## 🆘 Support

- 📖 **Documentation**: See [docs/](docs/) directory
- 🐛 **Issues**: [GitHub Issues](https://github.com/prubtsov/cv_to_osc_converter/issues)
- 💬 **Discussions**: [GitHub Discussions](https://github.com/prubtsov/cv_to_osc_converter/discussions)

## 📄 License

MIT License - see [LICENSE](docs/LICENSE) for details.

---

⭐ **Star this repo** if you find it useful!
