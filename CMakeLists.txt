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
    CVCalibrator.cpp
    PerformanceMonitor.cpp
    Config.cpp
    OSCSecurity.cpp
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
