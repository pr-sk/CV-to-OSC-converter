# CV to OSC Converter - GUI Development Guide

## Overview

The CV to OSC Converter now includes a comprehensive GUI application built with Dear ImGui, providing real-time visualization of CV signals and intuitive configuration management.

## Features

### 🎨 Kросс-платформенный GUI
- **Dear ImGui**: Modern immediate-mode GUI framework
- **OpenGL Rendering**: Hardware-accelerated graphics
- **GLFW Window Management**: Cross-platform window creation
- **Docking Support**: Flexible window layout system
- **Multiple Viewports**: Multi-monitor support

### 📊 Real-time Visualization CV сигналов
- **Live Signal Meters**: Real-time channel level display
- **Waveform Plotting**: Historical signal visualization with ImPlot
- **Multi-channel Display**: Up to 8 channels simultaneously
- **Configurable Time Windows**: 1-60 second history
- **Color-coded Channels**: Distinct visual identification
- **Zoom and Pan**: Interactive plot navigation

### 🔧 Drag-and-drop Configuration
- **Channel Reordering**: Drag channels to rearrange
- **OSC Address Assignment**: Drop OSC addresses onto channels
- **Preset Management**: Drag presets to apply configurations
- **Visual Feedback**: Clear drop zones and previews
- **Undo/Redo**: Configuration change history

### ⚡ Live Parameter Adjustment
- **Real-time Updates**: Changes apply immediately
- **Range Calibration**: Min/max voltage adjustment
- **OSC Mapping**: Live address modification
- **Enable/Disable**: Individual channel control
- **Gain Control**: Per-channel amplification
- **Filtering**: Real-time signal processing

## Installation

### Dependencies

#### macOS (Homebrew)
```bash
# GUI dependencies
brew install glfw imgui implot gl3w

# Core dependencies (if not already installed)
brew install portaudio liblo nlohmann-json cmake pkg-config
```

#### Ubuntu/Debian
```bash
# GUI dependencies
sudo apt-get install libglfw3-dev libgl1-mesa-dev libglu1-mesa-dev

# ImGui and ImPlot need to be built from source
git clone https://github.com/ocornut/imgui.git
git clone https://github.com/epezent/implot.git
# Follow their build instructions
```

#### Windows (vcpkg)
```bash
# GUI dependencies
vcpkg install glfw3 opengl imgui[glfw-binding,opengl3-binding] implot

# Core dependencies
vcpkg install portaudio liblo nlohmann-json
```

### Building

```bash
# Create build directory
mkdir build && cd build

# Configure with GUI support (default)
cmake .. -DBUILD_GUI=ON

# Or disable GUI if dependencies are missing
cmake .. -DBUILD_GUI=OFF

# Build both CLI and GUI versions
make -j$(nproc)

# Install
sudo make install
```

### Running

```bash
# GUI version
./cv_to_osc_converter_gui

# CLI version (unchanged)
./cv_to_osc_converter
```

## GUI Components

### Main Window
- **Control Panel**: Start/stop conversion, reset history
- **Status Display**: Audio device, OSC connection, message statistics
- **Channel Meters**: Real-time level visualization
- **Waveform Plot**: Historical signal display

### Configuration Windows
- **Channel Configuration**: Per-channel settings
- **OSC Configuration**: Network settings and status
- **Audio Configuration**: Device selection and parameters
- **Performance Monitor**: System performance metrics

### Menu System
- **File**: Load/save configurations
- **View**: Toggle window visibility
- **Control**: Start/stop operations, reset data
- **Help**: About dialog and documentation

## Usage Examples

### Basic Operation
1. **Launch GUI**: `./cv_to_osc_converter_gui`
2. **Configure Audio**: Select input device in Audio Configuration
3. **Set OSC Target**: Configure host/port in OSC Configuration
4. **Start Conversion**: Click "Start Conversion" or use Control menu
5. **Monitor Signals**: Watch real-time meters and waveforms

### Channel Configuration
1. **Open Channel Config**: View → Channel Configuration
2. **Set Names**: Double-click channel names to edit
3. **Adjust Ranges**: Use sliders for min/max voltage
4. **Configure OSC**: Set individual OSC addresses
5. **Visual Settings**: Customize plot colors and visibility

### Drag & Drop Operations
1. **Reorder Channels**: Drag channel headers to reorder
2. **Assign OSC Addresses**: Drag from preset list to channels
3. **Apply Presets**: Drag configuration presets to apply
4. **Visual Feedback**: Drop zones highlight when hovering

### Real-time Adjustment
1. **Live Range Changes**: Adjust min/max while running
2. **Enable/Disable**: Toggle channels without stopping
3. **Color Changes**: Modify plot colors in real-time
4. **OSC Routing**: Change addresses without reconnection

## Performance Features

### Optimized Rendering
- **60 FPS Target**: Smooth real-time updates
- **Adaptive Quality**: Reduces quality under load
- **Memory Management**: Efficient buffer allocation
- **GPU Acceleration**: Hardware-accelerated plotting

### Threading Model
- **Separate Worker Thread**: Audio processing isolation
- **Lock-free Updates**: Minimal GUI blocking
- **Background Processing**: Non-blocking configuration
- **Thread-safe Data**: Mutex-protected shared state

### Memory Efficiency
- **Circular Buffers**: Fixed memory for history
- **Pooled Allocations**: Reduced fragmentation
- **Lazy Loading**: On-demand resource allocation
- **Garbage Collection**: Automatic cleanup

## Customization

### Themes
```cpp
// Dark theme (default)
ImGui::StyleColorsDark();

// Light theme
ImGui::StyleColorsLight();

// Custom colors
setupCustomAudioTheme();
```

### Layout
- **Dockable Windows**: Arrange as needed
- **Save Layouts**: Persistent window arrangements
- **Multi-Monitor**: Spread across displays
- **Full Screen**: Dedicated visualization mode

### Keyboard Shortcuts
- **Ctrl+O**: Load configuration
- **Ctrl+S**: Save configuration
- **Space**: Start/stop conversion
- **R**: Reset channel history
- **F11**: Toggle fullscreen

## Troubleshooting

### Common Issues

#### GUI Fails to Start
```bash
# Check OpenGL support
glxinfo | grep "OpenGL version"

# Verify GLFW installation
pkg-config --libs glfw3
```

#### Poor Performance
- **Reduce History**: Lower time window
- **Disable Channels**: Hide unused channels
- **Lower FPS**: Adjust refresh rate
- **Close Windows**: Minimize open panels

#### Missing Features
- **Check Build**: Verify GUI was compiled
- **Update Dependencies**: Ensure latest versions
- **Graphics Drivers**: Update OpenGL drivers

### Debug Mode
```bash
# Build with debug info
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_GUI=ON
make -j$(nproc)

# Run with verbose output
./cv_to_osc_converter_gui --verbose
```

## Development

### Adding New GUI Components
1. **Create Widget**: Implement in GuiApplication.cpp
2. **Add Menu Item**: Register in renderMainMenuBar()
3. **Handle Events**: Process user interaction
4. **Update State**: Maintain data consistency

### Custom Visualizations
1. **Extend ImPlot**: Add new plot types
2. **Custom Drawing**: Use ImDrawList for graphics
3. **Animation**: Implement smooth transitions
4. **Interaction**: Add mouse/keyboard handling

### Integration Points
- **CVReader**: Audio input integration
- **OSCSender**: Network output integration
- **Config**: Settings management
- **ErrorHandler**: Status reporting

## Architecture

### Class Hierarchy
```
GuiApplication
├── CVReader (audio input)
├── OSCSender (network output)
├── Config (settings)
├── DragDropManager (UI interaction)
└── PerformanceMonitor (metrics)
```

### Data Flow
```
Audio Input → CVReader → GUI Thread → Visualization
                     ↓
OSC Output ← OSCSender ← Worker Thread ← Configuration
```

### Threading
- **Main Thread**: GUI rendering and event handling
- **Worker Thread**: Audio processing and OSC transmission
- **Synchronization**: Mutex-protected shared data

## Future Enhancements

### Planned Features
- **Plugin System**: Custom visualization plugins
- **Recording**: Save sessions to file
- **Scripting**: Lua/Python automation
- **Remote Control**: Web-based interface
- **MIDI Integration**: MIDI CC output

### Performance Improvements
- **Vulkan Rendering**: Next-gen graphics API
- **SIMD Optimization**: Vectorized processing
- **GPU Compute**: Shader-based processing
- **Memory Mapping**: Zero-copy buffers

## License

Same license as the main project. See LICENSE file for details.

## Contributing

GUI development follows the same contribution guidelines as the main project. Please ensure:

1. **ImGui Best Practices**: Use immediate-mode patterns
2. **Performance Testing**: Profile GUI additions
3. **Cross-platform**: Test on multiple platforms
4. **Documentation**: Update this guide for new features
