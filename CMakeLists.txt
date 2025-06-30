cmake_minimum_required(VERSION 3.12)
project(CVToOSCConverter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Performance optimizations
set(CMAKE_BUILD_TYPE Release)
if(APPLE)
    # Apple optimizations (works for both Intel and Apple Silicon)
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -ffast-math")
else()
    # Other platforms
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# macOS specific settings
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
endif()

# Find required packages
find_package(PkgConfig REQUIRED)

# Find PortAudio
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

# Find liblo (OSC library)
pkg_check_modules(LIBLO REQUIRED liblo)

# Find nlohmann/json
find_package(nlohmann_json REQUIRED)

# Add executable
add_executable(cv_to_osc_converter
    main.cpp
    CVReader.cpp
    OSCSender.cpp
    CommandLineInterface.cpp
    AudioDeviceManager.cpp
)

# Include directories  
target_include_directories(cv_to_osc_converter PRIVATE
    ${PORTAUDIO_INCLUDE_DIRS}
    ${LIBLO_INCLUDE_DIRS}
    /usr/local/Cellar/nlohmann-json/3.12.0/include
)

# Link libraries
target_link_libraries(cv_to_osc_converter
    ${PORTAUDIO_LIBRARIES}
    ${LIBLO_LIBRARIES}
    nlohmann_json::nlohmann_json
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

# macOS specific settings
if(APPLE)
    target_link_libraries(cv_to_osc_converter
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework AudioUnit"
        "-framework CoreServices"
    )
endif()

# Install target
install(TARGETS cv_to_osc_converter
    RUNTIME DESTINATION bin
)
