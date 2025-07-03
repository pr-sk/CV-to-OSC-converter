#!/bin/bash
# Install GUI dependencies for CV to OSC Converter

set -e  # Exit on any error

echo "🎨 Installing GUI dependencies for CV to OSC Converter..."
echo "=================================================="

# Check if we're on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo "❌ This script is for macOS only."
    echo "For other platforms, please refer to GUI_GUIDE.md"
    exit 1
fi

# Check if Homebrew is installed
if ! command -v brew &> /dev/null; then
    echo "❌ Homebrew is not installed. Please install it first:"
    echo "   /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
    exit 1
fi

echo "✅ Homebrew found"

# Update Homebrew
echo "📦 Updating Homebrew..."
brew update

# Install core dependencies (if not already installed)
echo "📦 Installing core dependencies..."
CORE_DEPS=(
    "cmake"
    "pkg-config" 
    "portaudio"
    "liblo"
    "nlohmann-json"
)

for dep in "${CORE_DEPS[@]}"; do
    if brew list "$dep" &>/dev/null; then
        echo "✅ $dep already installed"
    else
        echo "📦 Installing $dep..."
        brew install "$dep"
    fi
done

# Install GUI dependencies
echo "🎨 Installing GUI dependencies..."
GUI_DEPS=(
    "glfw"
    "glew"
)

for dep in "${GUI_DEPS[@]}"; do
    if brew list "$dep" &>/dev/null; then
        echo "✅ $dep already installed"
    else
        echo "📦 Installing $dep..."
        brew install "$dep"
    fi
done

# Install ImGui and ImPlot (these might need special handling)
echo "🎨 Installing ImGui and ImPlot..."

# Check if we can install imgui via brew
if brew search imgui | grep -q "imgui"; then
    if brew list imgui &>/dev/null; then
        echo "✅ imgui already installed"
    else
        echo "📦 Installing imgui..."
        brew install imgui
    fi
else
    echo "⚠️  ImGui not available via Homebrew. Will build from source."
    
    # Create a temporary directory for building
    TEMP_DIR=$(mktemp -d)
    cd "$TEMP_DIR"
    
    echo "📦 Cloning ImGui..."
    git clone https://github.com/ocornut/imgui.git
    cd imgui
    
    # Create a simple CMakeLists.txt for ImGui
    cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.12)
project(imgui)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW3 REQUIRED glfw3)
find_package(OpenGL REQUIRED)

add_library(imgui STATIC
    imgui.cpp
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    backends/imgui_impl_glfw.cpp
    backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/backends
)

target_link_libraries(imgui PUBLIC
    glfw
    OpenGL::GL
)

install(TARGETS imgui
    ARCHIVE DESTINATION lib
)

install(FILES
    imgui.h
    imconfig.h
    imgui_internal.h
    imstb_rectpack.h
    imstb_textedit.h
    imstb_truetype.h
    backends/imgui_impl_glfw.h
    backends/imgui_impl_opengl3.h
    DESTINATION include
)
EOF

    # Build and install ImGui
    mkdir build && cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=/opt/homebrew
    make -j$(sysctl -n hw.ncpu)
    sudo make install
    
    cd ../../..
    rm -rf "$TEMP_DIR"
    echo "✅ ImGui installed from source"
fi

# Install ImPlot
if brew search implot | grep -q "implot"; then
    if brew list implot &>/dev/null; then
        echo "✅ implot already installed"
    else
        echo "📦 Installing implot..."
        brew install implot
    fi
else
    echo "⚠️  ImPlot not available via Homebrew. Will build from source."
    
    # Create a temporary directory for building
    TEMP_DIR=$(mktemp -d)
    cd "$TEMP_DIR"
    
    echo "📦 Cloning ImPlot..."
    git clone https://github.com/epezent/implot.git
    cd implot
    
    # Create a simple CMakeLists.txt for ImPlot
    cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.12)
project(implot)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
find_path(IMGUI_INCLUDE_DIRS "imgui.h" HINTS /opt/homebrew/include /usr/local/include)

add_library(implot STATIC
    implot.cpp
    implot_demo.cpp
    implot_items.cpp
)

target_include_directories(implot PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${IMGUI_INCLUDE_DIRS}
)

install(TARGETS implot
    ARCHIVE DESTINATION lib
)

install(FILES
    implot.h
    implot_internal.h
    DESTINATION include
)
EOF

    # Build and install ImPlot
    mkdir build && cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=/opt/homebrew
    make -j$(sysctl -n hw.ncpu)
    sudo make install
    
    cd ../../..
    rm -rf "$TEMP_DIR"
    echo "✅ ImPlot installed from source"
fi

echo ""
echo "🎉 GUI dependencies installation complete!"
echo "=================================================="
echo ""
echo "📋 Installed dependencies:"
echo "   Core: cmake, pkg-config, portaudio, liblo, nlohmann-json"
echo "   GUI:  glfw, gl3w, imgui, implot"
echo ""
echo "🚀 You can now build the GUI version:"
echo "   mkdir build && cd build"
echo "   cmake .. -DBUILD_GUI=ON"
echo "   make -j\$(nproc)"
echo ""
echo "🎮 Run the GUI version:"
echo "   ./cv_to_osc_converter_gui"
echo ""
echo "📖 For more information, see GUI_GUIDE.md"
