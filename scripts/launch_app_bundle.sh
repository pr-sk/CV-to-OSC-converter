#!/bin/bash
# CV to OSC Converter App Bundle Launcher
# This script properly launches the macOS app bundle with permissions

set -e

APP_PATH="$(pwd)/CV to OSC Converter.app"
APP_EXECUTABLE="$APP_PATH/Contents/MacOS/cv_to_osc_converter_gui"
CLI_EXECUTABLE="$APP_PATH/Contents/MacOS/cv_to_osc_converter_cli"
APP_NAME="CV to OSC Converter"
BUNDLE_ID="com.cv-to-osc.converter"

echo "🎵 CV to OSC Converter App Bundle Launcher"
echo "==========================================="

# Check if app bundle exists
if [ ! -d "$APP_PATH" ]; then
    echo "❌ App bundle not found: $APP_PATH"
    exit 1
fi

# Check if executables exist
if [ ! -f "$APP_EXECUTABLE" ]; then
    echo "❌ GUI executable not found: $APP_EXECUTABLE"
    exit 1
fi

if [ ! -f "$CLI_EXECUTABLE" ]; then
    echo "❌ CLI executable not found: $CLI_EXECUTABLE"
    exit 1
fi

echo "📁 App Bundle: $APP_PATH"
echo "🔧 Bundle ID: $BUNDLE_ID"

# Function to check permissions using our built tool
check_permissions() {
    echo ""
    echo "🔐 Checking permissions..."
    
    # Try to run the permission check using the CLI executable
    if "$CLI_EXECUTABLE" --check-permissions 2>/dev/null; then
        return 0
    else
        return 1
    fi
}

# Function to request permissions
request_permissions() {
    echo ""
    echo "📝 Requesting permissions for app bundle..."
    echo "   Bundle ID: $BUNDLE_ID"
    echo "   This will show system permission dialogs."
    echo ""
    
    # Run the CLI app with permission request
    "$CLI_EXECUTABLE" --request-permissions &
    local pid=$!
    
    echo "⏳ Waiting for permission dialogs (30 seconds)..."
    sleep 30
    
    # Try to gracefully terminate
    if kill -0 $pid 2>/dev/null; then
        kill $pid 2>/dev/null || true
        wait $pid 2>/dev/null || true
    fi
}

# Function to launch the GUI app
launch_gui() {
    echo ""
    echo "🚀 Launching CV to OSC Converter GUI..."
    echo "   Use Cmd+Q or close window to quit"
    echo ""
    
    # Launch using macOS open command for proper app handling
    open "$APP_PATH"
}

# Function to launch CLI mode
launch_cli() {
    echo ""
    echo "💻 Launching CV to OSC Converter CLI..."
    echo "   Press Enter to stop"
    echo ""
    
    "$CLI_EXECUTABLE" "$@"
}

# Function to list devices
list_devices() {
    echo ""
    echo "🎤 Audio Devices:"
    "$CLI_EXECUTABLE" --list-devices
}

# Function to run diagnostics
run_diagnostics() {
    echo ""
    echo "🔍 Running detailed diagnostics..."
    "$CLI_EXECUTABLE" --list-devices --verbose
}

# Main menu
show_menu() {
    echo ""
    echo "Please choose an option:"
    echo "  1. 🔐 Check permissions"
    echo "  2. 📝 Request permissions"
    echo "  3. 🎤 List audio devices"
    echo "  4. 🔍 Run diagnostics"
    echo "  5. 🚀 Launch GUI application"
    echo "  6. 💻 Launch CLI application"
    echo "  7. ❓ Help"
    echo "  8. 🚪 Exit"
    echo ""
}

# Help function
show_help() {
    echo ""
    echo "🆘 CV to OSC Converter Help"
    echo "=========================="
    echo ""
    echo "This launcher helps you properly run the CV to OSC Converter app bundle."
    echo ""
    echo "First-time setup:"
    echo "  1. Choose option 2 to request permissions"
    echo "  2. Grant microphone access in the system dialog"
    echo "  3. Choose option 3 to verify devices are detected"
    echo "  4. Choose option 5 to launch the GUI"
    echo ""
    echo "Troubleshooting:"
    echo "  - If devices show as [UNAVAILABLE], run option 2"
    echo "  - If permission dialogs don't appear, try option 4 for diagnostics"
    echo "  - For detailed device info, use option 4"
    echo ""
    echo "Command line usage:"
    echo "  $0 [--gui|--cli|--check|--request|--list|--help]"
    echo ""
}

# Handle command line arguments
case "${1:-}" in
    --gui)
        launch_gui
        exit 0
        ;;
    --cli)
        shift
        launch_cli "$@"
        exit 0
        ;;
    --check)
        check_permissions
        exit 0
        ;;
    --request)
        request_permissions
        exit 0
        ;;
    --list)
        list_devices
        exit 0
        ;;
    --diagnostics)
        run_diagnostics
        exit 0
        ;;
    --help)
        show_help
        exit 0
        ;;
    "")
        # Interactive mode
        ;;
    *)
        echo "❌ Unknown option: $1"
        echo "Use --help for usage information"
        exit 1
        ;;
esac

# Interactive mode
while true; do
    show_menu
    read -p "Enter choice [1-8]: " choice
    
    case $choice in
        1)
            if check_permissions; then
                echo "✅ All permissions are granted!"
            else
                echo "⚠️  Some permissions may be missing. Try option 2 to request them."
            fi
            ;;
        2)
            request_permissions
            ;;
        3)
            list_devices
            ;;
        4)
            run_diagnostics
            ;;
        5)
            launch_gui
            break
            ;;
        6)
            launch_cli
            ;;
        7)
            show_help
            ;;
        8)
            echo "👋 Goodbye!"
            exit 0
            ;;
        *)
            echo "❌ Invalid choice. Please enter 1-8."
            ;;
    esac
    
    echo ""
    read -p "Press Enter to continue..."
done
