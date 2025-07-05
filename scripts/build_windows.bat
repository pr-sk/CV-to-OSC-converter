@echo off
REM CV to OSC Converter - Windows Build Script
REM This script builds the project using MSYS2/MinGW-w64

echo CV to OSC Converter - Windows Build Script
echo =============================================

REM Check if we're running in MSYS2 environment
if not defined MSYSTEM (
    echo Error: This script must be run from MSYS2 MinGW 64-bit shell
    echo Please install MSYS2 from https://www.msys2.org/
    echo Then open "MSYS2 MinGW 64-bit" and run this script
    pause
    exit /b 1
)

echo Detected MSYS2 environment: %MSYSTEM%

REM Check if required tools are available
echo Checking dependencies...

where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: CMake not found. Please install with:
    echo pacman -S mingw-w64-x86_64-cmake
    pause
    exit /b 1
)

where gcc >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: GCC not found. Please install with:
    echo pacman -S mingw-w64-x86_64-gcc
    pause
    exit /b 1
)

where pkg-config >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: pkg-config not found. Please install with:
    echo pacman -S mingw-w64-x86_64-pkg-config
    pause
    exit /b 1
)

echo ✓ Build tools found

REM Check if libraries are available
echo Checking libraries...

pkg-config --exists portaudio-2.0
if %errorlevel% neq 0 (
    echo Error: PortAudio not found. Please install with:
    echo pacman -S mingw-w64-x86_64-portaudio
    pause
    exit /b 1
)

pkg-config --exists liblo
if %errorlevel% neq 0 (
    echo Error: liblo not found. Please install with:
    echo pacman -S mingw-w64-x86_64-liblo
    pause
    exit /b 1
)

echo ✓ Required libraries found

REM Create build directory
if exist build (
    echo Cleaning previous build...
    rmdir /s /q build
)

echo Creating build directory...
mkdir build
cd build

REM Configure with CMake
echo Configuring with CMake...
cmake -G "MSYS Makefiles" ..
if %errorlevel% neq 0 (
    echo Error: CMake configuration failed
    pause
    exit /b 1
)

REM Build the project
echo Building project...
make -j%NUMBER_OF_PROCESSORS%
if %errorlevel% neq 0 (
    echo Error: Build failed
    pause
    exit /b 1
)

echo.
echo ✓ Build completed successfully!
echo.
echo Executable location: build/cv_to_osc_converter.exe
echo.
echo To run the converter:
echo   cd build
echo   ./cv_to_osc_converter.exe
echo.
echo To see all options:
echo   ./cv_to_osc_converter.exe --help
echo.

REM Test if the executable works
echo Testing executable...
if exist cv_to_osc_converter.exe (
    echo ✓ Executable found
    .\cv_to_osc_converter.exe --version 2>nul
    if %errorlevel% equ 0 (
        echo ✓ Executable runs successfully
    ) else (
        echo ! Executable may have runtime issues
    )
) else (
    echo Error: Executable not found
    exit /b 1
)

echo.
echo Build process completed!
pause
