cmake_minimum_required(VERSION 3.12)

# Get version information from Version.h or git
if(EXISTS "${CMAKE_SOURCE_DIR}/Version.h")
    file(READ "${CMAKE_SOURCE_DIR}/Version.h" VERSION_FILE_CONTENT)
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
    # Only build for current architecture to avoid linking issues
    # set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    
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

# Find OpenGL and GLFW only when building GUI
if(BUILD_GUI)
    find_package(OpenGL REQUIRED)
    # Use pkg-config to find GLFW and GLEW
    pkg_check_modules(GLFW3 REQUIRED glfw3)
    pkg_check_modules(GLEW REQUIRED glew)
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

# Add CLI executable (always built)
add_executable(cv_to_osc_converter
    main.cpp
    CVReader.cpp
    CVWriter.cpp
    OSCSender.cpp
    OSCReceiver.cpp
    OSCFormatManager.cpp
    CommandLineInterface.cpp
    AudioDeviceManager.cpp
    CVCalibrator.cpp
    PerformanceMonitor.cpp
    Config.cpp
    OSCSecurity.cpp
    ErrorHandler.cpp
    Localization.cpp
    ExternalDeviceManager.cpp
    DeviceManager.cpp
    MidiDeviceHandler.mm
    WiFiDeviceHandler.cpp
    PluginManager.cpp
    MacOSPermissions.mm
)

# Add GUI executable (if GUI libraries are available)
if(GUI_AVAILABLE)
    add_executable(cv_to_osc_converter_gui
        main_gui.cpp
        GuiApplication.cpp
        FileDialog.mm
        DragDropManager.cpp
        CommandSystem.cpp
        CVReader.cpp
        CVWriter.cpp
        OSCSender.cpp
        OSCReceiver.cpp
        OSCFormatManager.cpp
        CommandLineInterface.cpp
        AudioDeviceManager.cpp
        CVCalibrator.cpp
        PerformanceMonitor.cpp
        Config.cpp
        OSCSecurity.cpp
        ErrorHandler.cpp
        Localization.cpp
        ExternalDeviceManager.cpp
        DeviceManager.cpp
        MidiDeviceHandler.mm
        WiFiDeviceHandler.cpp
        PluginManager.cpp
        MacOSPermissions.mm
    )
endif()

# Include directories  
target_include_directories(cv_to_osc_converter PRIVATE
    ${PORTAUDIO_INCLUDE_DIRS}
    ${LIBLO_INCLUDE_DIRS}
)

# Add macOS-specific include paths only on macOS
if(APPLE)
    target_include_directories(cv_to_osc_converter PRIVATE
        /usr/local/Cellar/nlohmann-json/3.12.0/include
        /opt/homebrew/include
    )
endif()

# Link libraries
if(APPLE)
    # On macOS, let pkg-config handle framework dependencies to avoid duplicates
    target_link_libraries(cv_to_osc_converter PRIVATE
        ${PORTAUDIO_LIBRARIES}
        ${LIBLO_LIBRARIES}
        nlohmann_json::nlohmann_json
        "-framework Foundation"
        "-framework AVFoundation"
        "-framework AppKit"
        "-framework CoreMIDI"
    )
else()
    target_link_libraries(cv_to_osc_converter PRIVATE
        ${PORTAUDIO_LIBRARIES}
        ${LIBLO_LIBRARIES}
        nlohmann_json::nlohmann_json
    )
endif()

# Add version definitions
target_compile_definitions(cv_to_osc_converter PRIVATE
    GIT_COMMIT_HASH="${GIT_COMMIT_HASH}"
    GIT_BRANCH="${GIT_BRANCH}"
    BUILD_DATE="${BUILD_DATE}"
)

# Compiler flags
target_compile_options(cv_to_osc_converter PRIVATE
    ${PORTAUDIO_CFLAGS_OTHER}
    ${LIBLO_CFLAGS_OTHER}
)

# Link directories
target_link_directories(cv_to_osc_converter PRIVATE
    ${PORTAUDIO_LIBRARY_DIRS}
    ${LIBLO_LIBRARY_DIRS}
)


# Configure GUI executable if available
if(GUI_AVAILABLE)
    # Include directories for GUI
    target_include_directories(cv_to_osc_converter_gui PRIVATE
        ${PORTAUDIO_INCLUDE_DIRS}
        ${LIBLO_INCLUDE_DIRS}
        ${IMGUI_INCLUDE_DIRS}
        ${IMPLOT_INCLUDE_DIRS}
        ${GLFW3_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIRS}
    )
    
    # Add macOS-specific include paths for GUI
    if(APPLE)
        target_include_directories(cv_to_osc_converter_gui PRIVATE
            /usr/local/Cellar/nlohmann-json/3.12.0/include
            /opt/homebrew/include
        )
    endif()
    
    # Link libraries for GUI
    if(APPLE)
        target_link_libraries(cv_to_osc_converter_gui PRIVATE
            ${PORTAUDIO_LIBRARIES}
            ${LIBLO_LIBRARIES}
            nlohmann_json::nlohmann_json
            OpenGL::GL
            ${GLFW3_LIBRARIES}
            ${IMGUI_LIBRARY}
            ${IMPLOT_LIBRARY}
            ${GLEW_LIBRARIES}
            "-framework Foundation"
            "-framework AVFoundation"
            "-framework AppKit"
            "-framework CoreMIDI"
        )
    else()
        target_link_libraries(cv_to_osc_converter_gui PRIVATE
            ${PORTAUDIO_LIBRARIES}
            ${LIBLO_LIBRARIES}
            nlohmann_json::nlohmann_json
            OpenGL::GL
            ${GLFW3_LIBRARIES}
            ${IMGUI_LIBRARY}
            ${IMPLOT_LIBRARY}
            ${GLEW_LIBRARIES}
        )
    endif()
    
    # Add version definitions for GUI
    target_compile_definitions(cv_to_osc_converter_gui PRIVATE
        GIT_COMMIT_HASH="${GIT_COMMIT_HASH}"
        GIT_BRANCH="${GIT_BRANCH}"
        BUILD_DATE="${BUILD_DATE}"
    )
    
    # Compiler flags for GUI
    target_compile_options(cv_to_osc_converter_gui PRIVATE
        ${PORTAUDIO_CFLAGS_OTHER}
        ${LIBLO_CFLAGS_OTHER}
        ${GLFW3_CFLAGS_OTHER}
        ${GLEW_CFLAGS_OTHER}
    )
    
    # Link directories for GUI
    target_link_directories(cv_to_osc_converter_gui PRIVATE
        ${PORTAUDIO_LIBRARY_DIRS}
        ${LIBLO_LIBRARY_DIRS}
        ${GLFW3_LIBRARY_DIRS}
        ${GLEW_LIBRARY_DIRS}
    )
    
    # Install GUI target
    install(TARGETS cv_to_osc_converter_gui
        RUNTIME DESTINATION bin
    )
    
    message(STATUS "GUI version will be built as 'cv_to_osc_converter_gui'")
else()
    message(STATUS "Only CLI version will be built")
endif()

# Install CLI target
install(TARGETS cv_to_osc_converter
    RUNTIME DESTINATION bin
)

# Plugin options
option(BUILD_VST3 "Build VST3 plugin" ON)
option(BUILD_VST "Build VST2 plugin" ON) 
option(BUILD_AU "Build Audio Unit plugin" ON)
option(BUILD_PLUGINS "Build all plugins" ON)

# JUCE framework for plugin development
if(BUILD_PLUGINS OR BUILD_VST3 OR BUILD_VST OR BUILD_AU)
    # Check if JUCE exists in project directory
    if(EXISTS "${CMAKE_SOURCE_DIR}/JUCE/CMakeLists.txt")
        message(STATUS "JUCE found at: ${CMAKE_SOURCE_DIR}/JUCE")
        
        # Include JUCE plugin configuration
        include(plugins/JucePlugin.cmake)
        
        message(STATUS "Professional JUCE plugins will be built")
        
    else()
        message(WARNING "JUCE not found. Plugins will not be built.")
        message(STATUS "To build plugins, clone JUCE framework to ./JUCE/")
        message(STATUS "Run: git clone https://github.com/juce-framework/JUCE.git")
        set(BUILD_PLUGINS FALSE)
        set(BUILD_VST3 FALSE)
        set(BUILD_VST FALSE)
        set(BUILD_AU FALSE)
    endif()
endif()

# DMG packaging (macOS only)
if(APPLE)
    # Create app bundle for GUI version
    if(GUI_AVAILABLE)
        set_target_properties(cv_to_osc_converter_gui PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_BUNDLE_NAME "CV to OSC Converter"
            MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
            MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
            MACOSX_BUNDLE_IDENTIFIER "com.cvtoosc.converter"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.cvtoosc.converter"
            MACOSX_BUNDLE_ICON_FILE "AppIcon.icns"
            MACOSX_BUNDLE_INFO_STRING "CV to OSC Converter v${PROJECT_VERSION}"
            MACOSX_BUNDLE_COPYRIGHT "© 2025 CV to OSC Converter"
        )
        
        # Install app bundle
        install(TARGETS cv_to_osc_converter_gui
            BUNDLE DESTINATION .
        )
        
        # Install app icon if it exists
        if(EXISTS "${CMAKE_SOURCE_DIR}/resources/AppIcon.icns")
            install(FILES "${CMAKE_SOURCE_DIR}/resources/AppIcon.icns"
                DESTINATION "CV to OSC Converter.app/Contents/Resources"
            )
        endif()
    endif()
    
    # Custom target for DMG creation
    add_custom_target(dmg
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/create_dmg.sh
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Creating DMG package"
        DEPENDS cv_to_osc_converter
    )
    
    if(GUI_AVAILABLE)
        add_dependencies(dmg cv_to_osc_converter_gui)
    endif()
    
endif()
