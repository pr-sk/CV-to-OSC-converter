#!/bin/bash

# Professional DMG Creation Script for CV to OSC Converter
set -e

PROJECT_NAME="CV to OSC Converter"
VERSION="2.1.0"
DMG_NAME="CV-to-OSC-Converter-${VERSION}"
APP_NAME="cv_to_osc_converter_macos.app"

# Clean up old files
rm -f "${DMG_NAME}.dmg"
rm -rf dmg_staging

# Create staging directory
mkdir -p dmg_staging

echo "🔧 Preparing DMG contents..."

# Copy the main application
cp -R "build/${APP_NAME}" dmg_staging/
# Rename for user-friendly display
mv "dmg_staging/${APP_NAME}" "dmg_staging/CV to OSC Converter.app"

# Copy CLI version
cp build/cv_to_osc_converter dmg_staging/

# Copy documentation
cp README.md dmg_staging/
cp docs/LICENSE dmg_staging/

# Create Quick Start Guide
cat > dmg_staging/"Quick Start Guide.txt" << EOF
CV to OSC Converter v${VERSION}
==============================

🚀 GETTING STARTED:

1. INSTALLATION:
   • Drag "CV to OSC Converter.app" to Applications folder
   • Launch from Applications or Launchpad

2. FIRST RUN:
   • Connect your audio interface with CV inputs
   • Grant microphone access when prompted
   • The app will auto-configure with sensible defaults

3. CONFIGURATION:
   • OSC Target: Set destination IP and port (default: 127.0.0.1:9000)
   • Audio Device: Select your interface in preferences
   • CV Ranges: Configure voltage ranges per channel

4. OPERATION:
   • Connect CV signals to audio interface inputs
   • Click "Start Conversion" to begin
   • Monitor real-time signal levels and OSC output

5. COMMAND LINE:
   • Use "cv_to_osc_converter" for scripting/automation
   • Run with --help for all available options

⚡ FEATURES:
• Real-time CV to OSC conversion with sub-10ms latency
• Multi-channel support with individual range configuration
• Professional native macOS interface
• Bidirectional OSC communication
• JSON configuration with hot-reload
• Comprehensive signal monitoring

📋 REQUIREMENTS:
• macOS 10.13+ (optimized for Apple Silicon)
• Audio interface with line/instrument inputs
• Network connection for OSC communication

📖 For complete documentation, see README.md

© 2025 CV to OSC Converter - Professional Audio Tools
EOF

echo "📦 Creating professional DMG with create-dmg..."

# Create professional DMG with create-dmg
create-dmg \
  --volname "${PROJECT_NAME}" \
  --volicon "/Users/prubtsov/cv_to_osc_converter/assets/icon.icns" \
  --window-pos 200 120 \
  --window-size 800 400 \
  --icon-size 100 \
  --icon "CV to OSC Converter.app" 200 190 \
  --hide-extension "CV to OSC Converter.app" \
  --app-drop-link 600 190 \
  --icon "Quick Start Guide.txt" 400 300 \
  --icon "README.md" 500 300 \
  --icon "cv_to_osc_converter" 300 300 \
  --text-size 12 \
  --add-file "Quick Start Guide.txt" "dmg_staging/Quick Start Guide.txt" 400 300 \
  --add-file "README.md" "dmg_staging/README.md" 500 300 \
  --add-file "cv_to_osc_converter" "dmg_staging/cv_to_osc_converter" 300 300 \
  "${DMG_NAME}.dmg" \
  dmg_staging/

# Clean up
rm -rf dmg_staging

echo ""
echo "✅ Professional DMG created successfully!"
echo "📦 File: ${DMG_NAME}.dmg"
echo "📏 Size: $(du -h "${DMG_NAME}.dmg" | cut -f1)"

# Show file info
echo ""
echo "📊 DMG Information:"
file "${DMG_NAME}.dmg"

echo ""
echo "🎉 Distribution Package Ready!"
echo ""
echo "📝 Installation Instructions for Users:"
echo "1. Double-click ${DMG_NAME}.dmg to mount"
echo "2. Drag 'CV to OSC Converter.app' to Applications"
echo "3. Launch from Applications folder"
echo "4. Grant microphone permissions when prompted"
echo "5. Start converting CV signals to OSC!"

echo ""
echo "🔧 Additional Tools Included:"
echo "• cv_to_osc_converter - Command line version"
echo "• Quick Start Guide.txt - Essential setup information"
echo "• README.md - Complete documentation"
echo ""
echo "✨ Ready for professional distribution!"
