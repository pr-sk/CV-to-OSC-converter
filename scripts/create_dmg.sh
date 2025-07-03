#!/bin/bash

# CV to OSC Converter DMG Creation Script
# Creates a distributable DMG package for macOS

set -e  # Exit on any error

# Configuration
PROJECT_NAME="CV to OSC Converter"
VERSION="1.3.0"
DMG_NAME="CV-to-OSC-Converter-${VERSION}"
BUILD_DIR="${CMAKE_BINARY_DIR:-.}"
DIST_DIR="${BUILD_DIR}/dist"
DMG_DIR="${BUILD_DIR}/dmg"
APP_NAME="CV to OSC Converter.app"

echo "=========================================="
echo "Creating DMG for ${PROJECT_NAME} v${VERSION}"
echo "=========================================="

# Clean previous builds
echo "Cleaning previous builds..."
rm -rf "${DIST_DIR}"
rm -rf "${DMG_DIR}"
rm -f "${BUILD_DIR}/${DMG_NAME}.dmg"

# Create directories
echo "Creating directory structure..."
mkdir -p "${DIST_DIR}"
mkdir -p "${DMG_DIR}"

# Copy CLI executable
echo "Copying CLI executable..."
if [ -f "${BUILD_DIR}/cv_to_osc_converter" ]; then
    cp "${BUILD_DIR}/cv_to_osc_converter" "${DIST_DIR}/"
    echo "✓ CLI executable copied"
else
    echo "⚠ CLI executable not found"
fi

# Copy GUI app bundle if it exists
echo "Copying GUI application..."
if [ -d "${BUILD_DIR}/${APP_NAME}" ]; then
    cp -R "${BUILD_DIR}/${APP_NAME}" "${DIST_DIR}/"
    echo "✓ GUI application copied"
elif [ -f "${BUILD_DIR}/cv_to_osc_converter_gui" ]; then
    # Create app bundle manually if it doesn't exist
    echo "Creating app bundle from GUI executable..."
    mkdir -p "${DIST_DIR}/${APP_NAME}/Contents/MacOS"
    mkdir -p "${DIST_DIR}/${APP_NAME}/Contents/Resources"
    
    cp "${BUILD_DIR}/cv_to_osc_converter_gui" "${DIST_DIR}/${APP_NAME}/Contents/MacOS/"
    
    # Create Info.plist
    cat > "${DIST_DIR}/${APP_NAME}/Contents/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>cv_to_osc_converter_gui</string>
    <key>CFBundleIdentifier</key>
    <string>com.cvtoosc.converter</string>
    <key>CFBundleName</key>
    <string>CV to OSC Converter</string>
    <key>CFBundleVersion</key>
    <string>${VERSION}</string>
    <key>CFBundleShortVersionString</key>
    <string>${VERSION}</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.13</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSMicrophoneUsageDescription</key>
    <string>This application needs microphone access to read audio signals for CV conversion.</string>
</dict>
</plist>
EOF
    echo "✓ App bundle created"
else
    echo "⚠ GUI executable not found"
fi

# Copy plugins if they exist
echo "Copying plugins..."
PLUGIN_FOUND=false

# VST3 plugins
if [ -d "${BUILD_DIR}/cv_to_osc_converter_vst3.vst3" ]; then
    mkdir -p "${DIST_DIR}/Plugins/VST3"
    cp -R "${BUILD_DIR}/cv_to_osc_converter_vst3.vst3" "${DIST_DIR}/Plugins/VST3/"
    echo "✓ VST3 plugin copied"
    PLUGIN_FOUND=true
fi

# Audio Units
if [ -d "${BUILD_DIR}/cv_to_osc_converter_au.component" ]; then
    mkdir -p "${DIST_DIR}/Plugins/Components"
    cp -R "${BUILD_DIR}/cv_to_osc_converter_au.component" "${DIST_DIR}/Plugins/Components/"
    echo "✓ Audio Unit plugin copied"
    PLUGIN_FOUND=true
fi

if [ "$PLUGIN_FOUND" = false ]; then
    echo "⚠ No plugins found"
fi

# Copy documentation and resources
echo "Copying documentation..."
if [ -f "../README.md" ]; then
    cp "../README.md" "${DIST_DIR}/"
fi

if [ -f "../LICENSE" ]; then
    cp "../LICENSE" "${DIST_DIR}/"
fi

# Create Quick Start Guide
cat > "${DIST_DIR}/Quick Start Guide.txt" << EOF
CV to OSC Converter v${VERSION}
==============================

Quick Start:

1. STANDALONE APPLICATION:
   - Open "CV to OSC Converter.app" for the GUI version
   - Or use the command line tool "cv_to_osc_converter"

2. PLUGINS (if available):
   - VST3: Install from Plugins/VST3/ to ~/Library/Audio/Plug-Ins/VST3/
   - AU Component: Install from Plugins/Components/ to ~/Library/Audio/Plug-Ins/Components/

3. USAGE:
   - Connect audio interface with CV inputs
   - Configure OSC target (default: 127.0.0.1:9000)
   - Start conversion and monitor CV channels

4. REQUIREMENTS:
   - macOS 10.13 or later
   - Audio interface with line/instrument inputs
   - Network connection for OSC (local or remote)

For detailed documentation, see README.md

© 2025 CV to OSC Converter
EOF

# Create installer script
cat > "${DIST_DIR}/Install Plugins.command" << 'EOF'
#!/bin/bash
cd "$(dirname "$0")"

echo "CV to OSC Converter Plugin Installer"
echo "===================================="

if [ -d "Plugins/VST3" ]; then
    echo "Installing VST3 plugins..."
    mkdir -p ~/Library/Audio/Plug-Ins/VST3/
    cp -R Plugins/VST3/* ~/Library/Audio/Plug-Ins/VST3/
    echo "✓ VST3 plugins installed"
fi

if [ -d "Plugins/Components" ]; then
    echo "Installing Audio Unit components..."
    mkdir -p ~/Library/Audio/Plug-Ins/Components/
    cp -R Plugins/Components/* ~/Library/Audio/Plug-Ins/Components/
    echo "✓ Audio Units installed"
fi

echo ""
echo "Installation complete!"
echo "Please restart your DAW to load the new plugins."
read -p "Press Enter to continue..."
EOF

chmod +x "${DIST_DIR}/Install Plugins.command"

# Create uninstaller script
cat > "${DIST_DIR}/Uninstall Plugins.command" << 'EOF'
#!/bin/bash

echo "CV to OSC Converter Plugin Uninstaller"
echo "======================================"

if [ -f ~/Library/Audio/Plug-Ins/VST3/cv_to_osc_converter_vst3.vst3 ]; then
    echo "Removing VST3 plugin..."
    rm -rf ~/Library/Audio/Plug-Ins/VST3/cv_to_osc_converter_vst3.vst3
    echo "✓ VST3 plugin removed"
fi

if [ -f ~/Library/Audio/Plug-Ins/Components/cv_to_osc_converter_au.component ]; then
    echo "Removing Audio Unit component..."
    rm -rf ~/Library/Audio/Plug-Ins/Components/cv_to_osc_converter_au.component
    echo "✓ Audio Unit removed"
fi

echo ""
echo "Uninstallation complete!"
read -p "Press Enter to continue..."
EOF

chmod +x "${DIST_DIR}/Uninstall Plugins.command"

# Setup DMG staging directory
echo "Setting up DMG content..."
cp -R "${DIST_DIR}/." "${DMG_DIR}/"

# Create Applications symlink
ln -sf "/Applications" "${DMG_DIR}/Applications"

# Create DMG
echo "Creating DMG..."
hdiutil create -volname "${PROJECT_NAME}" \
    -srcfolder "${DMG_DIR}" \
    -ov -format UDZO \
    "${BUILD_DIR}/${DMG_NAME}.dmg"

# Cleanup
echo "Cleaning up..."
rm -rf "${DIST_DIR}"
rm -rf "${DMG_DIR}"

echo ""
echo "=========================================="
echo "✓ DMG created successfully!"
echo "File: ${BUILD_DIR}/${DMG_NAME}.dmg"
echo "=========================================="

# Show DMG info
echo ""
echo "DMG Contents:"
hdiutil attach "${BUILD_DIR}/${DMG_NAME}.dmg" -readonly -nobrowse -mountpoint /tmp/cv_osc_dmg > /dev/null
find /tmp/cv_osc_dmg -type f -exec basename {} \; | sort
hdiutil detach /tmp/cv_osc_dmg > /dev/null

echo ""
echo "Ready for distribution!"
