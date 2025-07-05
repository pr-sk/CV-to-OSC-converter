#!/bin/bash
# CV to OSC Converter GUI Launcher

echo "🎵 CV to OSC Converter GUI Launcher"
echo "=================================="

# Check if we're in a GUI environment
if [ -z "$DISPLAY" ] && [ "$TERM_PROGRAM" != "Apple_Terminal" ] && [ "$TERM_PROGRAM" != "iTerm.app" ]; then
    echo "❌ No GUI environment detected!"
    echo ""
    echo "Please run this script in one of the following ways:"
    echo ""
    echo "1. 🖥️  Direct access: Open Terminal.app on the Mac and run:"
    echo "   cd /Users/prubtsov/cv_to_osc_converter"
    echo "   ./launch_gui.sh"
    echo ""
    echo "2. 🔗 Remote access: Use VNC or Screen Sharing to connect to the Mac"
    echo ""
    echo "3. 📱 SSH with X11: Set up X11 forwarding (requires XQuartz)"
    echo "   ssh -X username@mac-ip"
    echo "   cd /Users/prubtsov/cv_to_osc_converter"
    echo "   ./launch_gui.sh"
    echo ""
    exit 1
fi

# Change to build directory
cd "$(dirname "$0")/build" || {
    echo "❌ Build directory not found!"
    echo "Please run: mkdir build && cd build && cmake .. -DBUILD_GUI=ON && make"
    exit 1
}

# Check if GUI executable exists
if [ ! -f "./cv_to_osc_converter_gui" ]; then
    echo "❌ GUI executable not found!"
    echo "Please build the GUI version first:"
    echo "   cd build"
    echo "   cmake .. -DBUILD_GUI=ON"
    echo "   make -j$(sysctl -n hw.ncpu)"
    exit 1
fi

echo "🚀 Starting CV to OSC Converter GUI..."
echo "📍 Working directory: $(pwd)"
echo ""

# Set up environment for GUI libraries
export PKG_CONFIG_PATH="/usr/local/Cellar/glfw/3.4/lib/pkgconfig:/usr/local/Cellar/glew/2.2.0_1/lib/pkgconfig:$PKG_CONFIG_PATH"

# Launch the GUI application
exec ./cv_to_osc_converter_gui
