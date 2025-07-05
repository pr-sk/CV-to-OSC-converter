cmake_minimum_required(VERSION 3.12)

# Get version information from Version.h or git
if(EXISTS "${CMAKE_SOURCE_DIR}/src/config/Version.h")
    file(READ "${CMAKE_SOURCE_DIR}/src/config/Version.h" VERSION_FILE_CONTENT)
    string(REGEX MATCH "#define CV_TO_OSC_VERSION_MAJOR ([0-9]+)" _ "${VERSION_FILE_CONTENT}")
    set(PROJECT_VERSION_MAJOR "${CMAKE_MATCH_1}")
    string(REGEX MATCH "#define CV_TO_OSC_VERSION_MINOR ([0-9]+)" _ "${VERSION_FILE_CONTENT}")
    set(PROJECT_VERSION_MINOR "${CMAKE_MATCH_1}")
    string(REGEX MATCH "#define CV_TO_OSC_VERSION_PATCH ([0-9]+)" _ "${VERSION_FILE_CONTENT}")
    set(PROJECT_VERSION_PATCH "${CMAKE_MATCH_1}")
    set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
else()
    set(PROJECT_VERSION "1.0.0")
endif()

project(CVToOSCConverter VERSION ${PROJECT_VERSION})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Get git information for version
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
else()
    set(GIT_COMMIT_HASH "unknown")
    set(GIT_BRANCH "unknown")
endif()

# Allow override from environment or command line
if(DEFINED ENV{GIT_COMMIT_HASH})
    set(GIT_COMMIT_HASH $ENV{GIT_COMMIT_HASH})
endif()
if(DEFINED ENV{GIT_BRANCH})
    set(GIT_BRANCH $ENV{GIT_BRANCH})
endif()

# Set build date
string(TIMESTAMP BUILD_DATE "%Y-%m-%d %H:%M:%S UTC" UTC)
if(DEFINED ENV{BUILD_DATE})
    set(BUILD_DATE $ENV{BUILD_DATE})
endif()

# Performance optimizations
set(CMAKE_BUILD_TYPE Release)
if(APPLE)
    # Apple optimizations (works for both Intel and Apple Silicon)
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
else()
    # Other platforms
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Platform-specific settings
if(APPLE)
    # Get current architecture
    execute_process(
        COMMAND uname -m
        OUTPUT_VARIABLE CURRENT_ARCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    message(STATUS "🔍 Detected architecture: ${CURRENT_ARCH}")
    
    # Force arm64 architecture only - no x86_64 support
    set(CMAKE_OSX_ARCHITECTURES "arm64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -arch arm64")
    
    # Add additional flags to prevent x86_64
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch arm64")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -arch arm64")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -arch arm64")
    
    # Find the macOS SDK
    execute_process(
        COMMAND xcrun --show-sdk-path
        OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    if(CMAKE_OSX_SYSROOT)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isysroot ${CMAKE_OSX_SYSROOT}")
    endif()
elseif(WIN32)
    # Windows-specific settings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN32_LEAN_AND_MEAN -DNOMINMAX")
    
    # Enable Unicode support
    add_definitions(-DUNICODE -D_UNICODE)
    
    # Set Windows version for API compatibility
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7 and later
    
    # Suppress common Windows warnings
    if(MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4996 /wd4267 /wd4244")
    endif()
endif()

# Find required packages
find_package(PkgConfig REQUIRED)

# Find PortAudio
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

# Find liblo (OSC library)
pkg_check_modules(LIBLO REQUIRED liblo)

# Remove duplicate libraries to fix linker warnings
if(PORTAUDIO_LIBRARIES)
    list(REMOVE_DUPLICATES PORTAUDIO_LIBRARIES)
endif()
if(LIBLO_LIBRARIES)
    list(REMOVE_DUPLICATES LIBLO_LIBRARIES)
endif()

# Find nlohmann/json
find_package(nlohmann_json REQUIRED)

# Always find OpenGL for potential GUI use
find_package(OpenGL)

# Find OpenGL and GLFW only when building GUI
if(BUILD_GUI)
    if(OpenGL_FOUND)
        # Use pkg-config to find GLFW and GLEW
        pkg_check_modules(GLFW3 REQUIRED glfw3)
        pkg_check_modules(GLEW REQUIRED glew)
    else()
        message(WARNING "OpenGL not found. GUI will be built without OpenGL support.")
    endif()
endif()

# Check for GUI libraries availability
option(BUILD_GUI "Build GUI version" ON)

if(BUILD_GUI)
    # Try to find ImGui and ImPlot
    find_path(IMGUI_INCLUDE_DIRS "imgui.h" HINTS /usr/local/include/imgui /opt/homebrew/include/imgui /usr/local/include /opt/homebrew/include)
    find_path(IMPLOT_INCLUDE_DIRS "implot.h" HINTS /usr/local/include/implot /opt/homebrew/include/implot /usr/local/include /opt/homebrew/include)
    find_library(IMGUI_LIBRARY imgui HINTS /usr/local/lib /opt/homebrew/lib)
    find_library(IMPLOT_LIBRARY implot HINTS /usr/local/lib /opt/homebrew/lib)
    
    if(IMGUI_INCLUDE_DIRS AND IMPLOT_INCLUDE_DIRS AND IMGUI_LIBRARY AND IMPLOT_LIBRARY)
        set(GUI_AVAILABLE TRUE)
        message(STATUS "ImGui found at: ${IMGUI_INCLUDE_DIRS}")
        message(STATUS "ImPlot found at: ${IMPLOT_INCLUDE_DIRS}")
        message(STATUS "ImGui library: ${IMGUI_LIBRARY}")
        message(STATUS "ImPlot library: ${IMPLOT_LIBRARY}")
    else()
        set(GUI_AVAILABLE FALSE)
        message(WARNING "ImGui/ImPlot not found. GUI version will not be built.")
        message(STATUS "Please run: ./install_gui_deps.sh")
    endif()
else()
    set(GUI_AVAILABLE FALSE)
endif()

# Only build native macOS application

# Add native macOS GUI executable (always available on Apple)
if(APPLE)
    set(MACOS_SOURCES
        src/gui/main_professional_osc_mixer.mm
        src/gui/ProfessionalOSCMixer.mm
        src/gui/ProfessionalMixerWindow.mm
        src/gui/DeviceConfigurationDialogs.mm
        src/core/OSCMixerEngine.cpp
        src/core/AudioDeviceIntegration.cpp
        src/core/RealAudioStream.cpp
        src/audio/CVReader.cpp
        src/audio/CVWriter.cpp
        src/osc/OSCSender.cpp
        src/osc/OSCReceiver.cpp
        src/osc/OSCFormatManager.cpp
        src/osc/OSCSenderEnhanced.cpp
        src/osc/OSCTransport.cpp
        src/osc/OSCUDPTransport.cpp
        src/osc/OSCTCPTransport.cpp
        src/core/Config.cpp
        src/osc/OSCSecurity.cpp
        src/core/ErrorHandler.cpp
        src/core/AudioDeviceManager.cpp
        src/audio/CVCalibrator.cpp
        src/core/PerformanceMonitor.cpp
        src/utils/ExternalDeviceManager.cpp
        src/utils/ExternalDeviceMapper.cpp
        src/core/DeviceManager.cpp
        src/utils/WiFiDeviceHandler.cpp
        src/utils/PluginManager.cpp
        src/platform/MidiDeviceHandler.mm
        src/platform/MacOSPermissions.mm
        src/utils/Localization.cpp
    )
    
    add_executable(professional_osc_mixer ${MACOS_SOURCES})
    set(GUI_AVAILABLE TRUE)
endif()

# Include directories for macOS application
if(APPLE)
    target_include_directories(professional_osc_mixer PRIVATE
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_SOURCE_DIR}/src/audio
        ${CMAKE_SOURCE_DIR}/src/osc
        ${CMAKE_SOURCE_DIR}/src/gui
        ${CMAKE_SOURCE_DIR}/src/utils
        ${CMAKE_SOURCE_DIR}/src/platform
        ${CMAKE_SOURCE_DIR}/src/config
        ${PORTAUDIO_INCLUDE_DIRS}
        ${LIBLO_INCLUDE_DIRS}
        /usr/local/Cellar/nlohmann-json/3.12.0/include
        /opt/homebrew/include
    )
endif()

# Link libraries for macOS application - force arm64 versions
if(APPLE)
    # Force linking to arm64 versions of libraries
    find_library(PORTAUDIO_ARM64_LIB portaudio PATHS /opt/homebrew/lib NO_DEFAULT_PATH)
    find_library(LIBLO_ARM64_LIB lo PATHS /opt/homebrew/lib NO_DEFAULT_PATH)
    
    if(PORTAUDIO_ARM64_LIB AND LIBLO_ARM64_LIB)
        target_link_libraries(professional_osc_mixer PRIVATE
            ${PORTAUDIO_ARM64_LIB}
            ${LIBLO_ARM64_LIB}
            nlohmann_json::nlohmann_json
            "-framework Foundation"
            "-framework AVFoundation"
            "-framework AppKit"
            "-framework CoreMIDI"
            "-framework Cocoa"
            "-framework QuartzCore"
            "-framework CoreAudio"
            "-framework AudioToolbox"
            "-framework AudioUnit"
            "-framework CoreFoundation"
            "-framework CoreServices"
        )
        message(STATUS "Using arm64 libraries: ${PORTAUDIO_ARM64_LIB}, ${LIBLO_ARM64_LIB}")
    else()
        # Fallback to system libraries if arm64 versions not found
        target_link_libraries(professional_osc_mixer PRIVATE
            portaudio
            lo
            nlohmann_json::nlohmann_json
            "-framework Foundation"
            "-framework AVFoundation"
            "-framework AppKit"
            "-framework CoreMIDI"
            "-framework Cocoa"
            "-framework QuartzCore"
            "-framework CoreAudio"
            "-framework AudioToolbox"
            "-framework AudioUnit"
            "-framework CoreFoundation"
            "-framework CoreServices"
        )
        message(WARNING "arm64 libraries not found in /opt/homebrew/lib, using system libraries")
    endif()
endif()

# Add version definitions for macOS app
if(APPLE)
    target_compile_definitions(professional_osc_mixer PRIVATE
        GIT_COMMIT_HASH="${GIT_COMMIT_HASH}"
        GIT_BRANCH="${GIT_BRANCH}"
        BUILD_DATE="${BUILD_DATE}"
    )
endif()

# Compiler flags for macOS app
if(APPLE)
    target_compile_options(professional_osc_mixer PRIVATE
        ${PORTAUDIO_CFLAGS_OTHER}
        ${LIBLO_CFLAGS_OTHER}
    )
    
    # Link directories for macOS app
    target_link_directories(professional_osc_mixer PRIVATE
        ${PORTAUDIO_LIBRARY_DIRS}
        ${LIBLO_LIBRARY_DIRS}
        /usr/local/lib
        /opt/homebrew/lib
    )
endif()

# App bundle configuration for macOS
if(APPLE)
    set_target_properties(professional_osc_mixer PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/resources/Info.plist
        MACOSX_BUNDLE_BUNDLE_NAME "Professional OSC Mixer"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_IDENTIFIER "com.cvtoosc.professional-mixer"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.cvtoosc.professional-mixer"
        MACOSX_BUNDLE_ICON_FILE "AppIcon.icns"
        MACOSX_BUNDLE_INFO_STRING "Professional OSC Mixer v${PROJECT_VERSION}"
        MACOSX_BUNDLE_COPYRIGHT "© 2025 Professional OSC Mixer"
    )
    
    # Install app bundle
    install(TARGETS professional_osc_mixer
        BUNDLE DESTINATION .
    )
    
    # Install app icon if it exists
    if(EXISTS "${CMAKE_SOURCE_DIR}/resources/AppIcon.icns")
        install(FILES "${CMAKE_SOURCE_DIR}/resources/AppIcon.icns"
            DESTINATION "Professional OSC Mixer.app/Contents/Resources"
        )
    endif()
    
    message(STATUS "✅ Professional OSC Mixer will be built as native macOS app (arm64 only)")
    message(STATUS "🚫 x86_64 architecture is explicitly disabled")
    message(STATUS "📦 Using arm64 libraries: PortAudio and liblo from /opt/homebrew")
else()
    message(FATAL_ERROR "This project only supports macOS with Apple Silicon (arm64)")
endif()

# Plugin options (disabled)
option(BUILD_VST3 "Build VST3 plugin" OFF)
option(BUILD_VST "Build VST2 plugin" OFF) 
option(BUILD_AU "Build Audio Unit plugin" OFF)
option(BUILD_PLUGINS "Build all plugins" OFF)

# JUCE framework disabled
if(FALSE) # Plugins completely disabled
    message(STATUS "Plugins are disabled")
endif()

# DMG packaging for Professional OSC Mixer
if(APPLE)
    # Custom target for DMG creation
    add_custom_target(dmg
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/create_professional_dmg.sh
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Creating Professional OSC Mixer DMG package"
        DEPENDS professional_osc_mixer
    )
endif()

# Test executable
add_executable(test_audio_input 
    test_audio_input.cpp
    src/osc/OSCSender.cpp
    src/core/ErrorHandler.cpp
)

target_include_directories(test_audio_input PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/audio
    ${CMAKE_SOURCE_DIR}/src/osc
    ${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/utils
    ${CMAKE_SOURCE_DIR}/src/platform
    ${CMAKE_SOURCE_DIR}/src/config
    ${PORTAUDIO_INCLUDE_DIRS}
    ${LIBLO_INCLUDE_DIRS}
    /usr/local/Cellar/nlohmann-json/3.12.0/include
    /opt/homebrew/include
)

target_link_directories(test_audio_input PRIVATE
    ${PORTAUDIO_LIBRARY_DIRS}
    ${LIBLO_LIBRARY_DIRS}
    /usr/local/lib
    /opt/homebrew/lib
)

# Use arm64 libraries for test_audio_input too
if(PORTAUDIO_ARM64_LIB AND LIBLO_ARM64_LIB)
    target_link_libraries(test_audio_input PRIVATE
        ${PORTAUDIO_ARM64_LIB}
        ${LIBLO_ARM64_LIB}
        nlohmann_json::nlohmann_json
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework CoreFoundation"
        "-framework CoreMIDI"
        "-framework Cocoa"
    )
else()
    target_link_libraries(test_audio_input PRIVATE
        portaudio
        lo
        nlohmann_json::nlohmann_json
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework CoreFoundation"
        "-framework CoreMIDI"
        "-framework Cocoa"
    )
endif()
