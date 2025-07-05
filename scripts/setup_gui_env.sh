#!/bin/bash
# Setup environment variables for GUI build

echo "🔧 Setting up environment for GUI build..."

# Add PKG_CONFIG_PATH for GUI dependencies
export PKG_CONFIG_PATH="/usr/local/Cellar/glfw/3.4/lib/pkgconfig:/usr/local/Cellar/glew/2.2.0_1/lib/pkgconfig:$PKG_CONFIG_PATH"

# Also try to find dependencies in common locations
if [ -d "/opt/homebrew/lib/pkgconfig" ]; then
    export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:$PKG_CONFIG_PATH"
fi

echo "✅ PKG_CONFIG_PATH set to: $PKG_CONFIG_PATH"

# Verify that we can find the dependencies
echo "🔍 Verifying GUI dependencies..."

if pkg-config --exists glfw3; then
    echo "✅ GLFW3 found: $(pkg-config --modversion glfw3)"
else
    echo "❌ GLFW3 not found"
fi

if pkg-config --exists glew; then
    echo "✅ GLEW found: $(pkg-config --modversion glew)"
else
    echo "❌ GLEW not found"
fi

if [ -f "/usr/local/lib/libimgui.a" ]; then
    echo "✅ ImGui library found"
else
    echo "❌ ImGui library not found"
fi

if [ -f "/usr/local/lib/libimplot.a" ]; then
    echo "✅ ImPlot library found"
else
    echo "❌ ImPlot library not found"
fi

echo ""
echo "🚀 Ready to build! Run:"
echo "   cd build"
echo "   cmake .. -DBUILD_GUI=ON"
echo "   make -j\$(sysctl -n hw.ncpu)"
