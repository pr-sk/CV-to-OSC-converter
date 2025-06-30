#!/bin/bash

# CV to OSC Converter - Automated Test Runner
# This script builds and runs all tests for the CV to OSC converter

set -e  # Exit on any error

echo "CV to OSC Converter - Automated Test Runner"
echo "============================================"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    print_error "CMakeLists.txt not found. Please run this script from the project root directory."
    exit 1
fi

# Create build directory for tests
TEST_BUILD_DIR="build_tests"
print_status "Creating test build directory: $TEST_BUILD_DIR"
mkdir -p "$TEST_BUILD_DIR"
cd "$TEST_BUILD_DIR"

# Build the simple test executable
print_status "Building simple test executable..."
g++ -std=c++17 -O2 -Wall -Wextra \
    -I/usr/local/Cellar/nlohmann-json/3.12.0/include \
    ../tests/simple_test.cpp \
    -o simple_test

if [ $? -eq 0 ]; then
    print_success "Test executable built successfully"
else
    print_error "Failed to build test executable"
    exit 1
fi

# Run the simple tests
print_status "Running automated tests..."
echo ""

if ./simple_test; then
    print_success "All tests completed successfully!"
    TEST_RESULT=0
else
    print_error "Some tests failed!"
    TEST_RESULT=1
fi

echo ""
print_status "Test execution completed"

# Cleanup
cd ..
print_status "Cleaning up test build directory"
rm -rf "$TEST_BUILD_DIR"

# Final status
if [ $TEST_RESULT -eq 0 ]; then
    print_success "✅ All tests passed! The CV to OSC converter is working correctly."
else
    print_error "❌ Tests failed! Please check the output above for details."
fi

exit $TEST_RESULT
