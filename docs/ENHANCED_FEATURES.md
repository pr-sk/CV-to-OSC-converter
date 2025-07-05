# Enhanced Features - CV to OSC Converter

## 🆕 New Features Added

### 1. Enhanced CV Signal Visualization

#### Individual Channel Windows
- **Separate windows for each CV channel** with detailed visualization
- Access via `View → Individual Channel Windows` in the menu
- Each window shows:
  - Real-time signal plot with advanced zoom controls
  - Channel status and configuration
  - Large-scale level meters
  - Current value display with voltage range

#### Advanced Zoom and Scaling Controls
- **Zoom Level**: 0.1x to 10x magnification for detailed signal inspection
- **Time Range**: 1 to 60 seconds of signal history
- **Auto-scale Y-axis**: Automatically adjusts voltage range based on channel settings
- **Manual scaling**: Custom voltage range from -20V to +20V
- **Reset zoom** button for quick return to default view

#### Visualization Features
- **Real-time plotting** with ImPlot for smooth, professional visualization
- **Custom color coding** for each channel
- **Current value markers** highlighted on plots
- **Progress bar meters** showing normalized signal levels
- **Multi-window support** - view multiple channels simultaneously

### 2. OSC to CV Conversion (Bidirectional Communication)

#### OSC Message Reception
- **Incoming OSC support** - Convert OSC messages back to CV signals
- **Dedicated OSC listening port** (default: 8001, configurable)
- **Real-time OSC to CV conversion** with immediate visualization updates

#### OSC Message Format
The application expects OSC messages in the following format:
```
/cv/channel/1 <float_value>    # Channel 1
/cv/channel/2 <float_value>    # Channel 2
/cv/channel/N <float_value>    # Channel N
```

**Value Range**: 0.0 to 1.0 (normalized), automatically scaled to channel's voltage range

#### Configuration
- **Listen Port**: Configurable in OSC Configuration window
- **Start/Stop controls** for OSC listening service
- **Status indicators** showing listening state
- **Error handling** with detailed logging

## 🎛️ Usage Instructions

### Enhanced Visualization

1. **Open Individual Channel Windows**:
   - Go to `View → Individual Channel Windows`
   - Select specific channels or "Show All Individual Windows"

2. **Zoom Controls**:
   - Use zoom slider (0.1x to 10x) for magnification
   - Adjust time range (1-60 seconds) for history length
   - Toggle auto-scale or set manual voltage limits

3. **Window Management**:
   - Each channel window can be moved and resized independently
   - Close windows individually or via View menu
   - Windows remember their state during session

### OSC to CV Conversion

1. **Enable OSC Listening**:
   - Open "OSC Configuration" window
   - Set desired listen port (default: 8001)
   - Click "Start OSC Listening"

2. **Send OSC Messages**:
   ```bash
   # Example using oscsend (if available)
   oscsend localhost 8001 /cv/channel/1 f 0.5   # Set channel 1 to 50%
   oscsend localhost 8001 /cv/channel/2 f 0.8   # Set channel 2 to 80%
   ```

3. **Monitor Conversion**:
   - Watch real-time updates in channel visualizations
   - Check console output for conversion logs
   - Status indicators show connection state

## 🔧 Technical Details

### Visualization Architecture
- **ImPlot Integration**: Professional plotting library for real-time visualization
- **Thread-safe Updates**: Mutex-protected data structures for concurrent access
- **Efficient Rendering**: Optimized for 60+ FPS with multiple windows
- **Memory Management**: Circular buffers limit history to 1000 samples per channel

### OSC Reception System
- **liblo Integration**: Professional OSC library for message handling
- **Multi-threaded**: Non-blocking OSC message reception
- **Format Flexibility**: Supports float, int, and mixed-type OSC messages
- **Error Handling**: Comprehensive error reporting and recovery

### Data Flow
```
OSC Message → OSC Receiver → Channel Data → Real-time Visualization
     ↓
CV Output (future enhancement)
```

## 📊 Performance Optimizations

- **Efficient Data Structures**: `std::deque` for O(1) history management
- **Selective Rendering**: Only active windows consume GPU resources
- **Memory Bounds**: Limited sample history prevents memory bloat
- **Lock-free Reads**: Minimal synchronization overhead

## 🎨 UI/UX Improvements

- **Professional Color Scheme**: Optimized for audio/CV work environments
- **Intuitive Controls**: Standard zoom/pan interactions
- **Status Indicators**: Clear visual feedback for all operations
- **Keyboard Shortcuts**: Efficient workflow with hotkeys
- **Window Management**: Flexible layout with docking support

## 🔍 Debugging and Monitoring

- **Console Logging**: Detailed operation logs for troubleshooting
- **Performance Metrics**: Real-time FPS, CPU, and memory monitoring
- **Connection Testing**: Built-in OSC connection verification
- **Error Reporting**: User-friendly error messages with suggestions

## 🚀 Future Enhancements

- **CV Output Hardware**: Physical CV output via audio interfaces
- **MIDI Integration**: MIDI CC to CV conversion
- **Preset Management**: Save/load visualization and routing presets
- **Advanced Filtering**: Low-pass, high-pass, and custom signal filters
- **Recording/Playback**: Capture and replay CV sequences

## 💡 Tips and Best Practices

1. **Use Individual Windows** for detailed analysis of specific channels
2. **Adjust zoom levels** based on signal frequency content
3. **Enable auto-scale** for signals with unknown voltage ranges
4. **Monitor OSC listening status** to ensure proper connectivity
5. **Check console output** for detailed operation information

## 🐛 Troubleshooting

### OSC Reception Issues
- Verify port availability (no other applications using the port)
- Check firewall settings for incoming OSC messages
- Ensure OSC message format matches expected pattern
- Monitor console for error messages

### Visualization Performance
- Close unused individual channel windows
- Reduce zoom levels for better performance
- Limit time range for real-time applications
- Check GPU drivers for optimal rendering

---

*Enhanced features implemented: Individual channel visualization windows, advanced zoom controls, OSC to CV conversion, real-time bidirectional communication.*
