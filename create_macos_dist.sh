#!/bin/bash
# Create macOS distribution package for CV to OSC Converter

echo "📦 Creating macOS distribution package..."
echo "========================================"

# Create distribution directory
DIST_DIR="dist/macOS"
mkdir -p "$DIST_DIR"

# Copy the App Bundle
echo "📋 Copying App Bundle..."
cp -R "CV to OSC Converter.app" "$DIST_DIR/"

# Create a DMG file (optional, requires create-dmg)
if command -v create-dmg &> /dev/null; then
    echo "💿 Creating DMG file..."
    create-dmg \
        --volname "CV to OSC Converter" \
        --volicon "assets/icon.icns" \
        --window-pos 200 120 \
        --window-size 600 300 \
        --icon-size 100 \
        --icon "CV to OSC Converter.app" 175 120 \
        --hide-extension "CV to OSC Converter.app" \
        --app-drop-link 425 120 \
        "$DIST_DIR/CV-to-OSC-Converter-v1.0.0.dmg" \
        "$DIST_DIR/"
else
    echo "⚠️  create-dmg not found, skipping DMG creation"
    echo "   Install with: brew install create-dmg"
fi

# Create documentation
echo "📚 Creating documentation..."
cp README.md "$DIST_DIR/"
cp GUI_LAUNCH_INSTRUCTIONS.md "$DIST_DIR/"

# Create quick start guide
cat > "$DIST_DIR/Quick Start Guide.txt" << 'EOF'
CV to OSC Converter - Quick Start Guide
========================================

🚀 LAUNCHING THE APPLICATION:
1. Double-click "CV to OSC Converter.app"
2. Or drag it to Applications folder and launch from there

🎛️ FIRST-TIME SETUP:
1. Connect your CV sources to audio interface inputs
2. Launch the application
3. Go to View → Audio Configuration to select your audio device
4. Go to View → OSC Configuration to set target host/port
5. Click "Start Conversion" to begin CV → OSC conversion

📊 USING THE GUI:
- Main Window: Real-time visualization and control
- Channel Configuration: Set voltage ranges and OSC addresses  
- Performance Monitor: Track system performance
- All windows are dockable and resizable

🔧 TROUBLESHOOTING:
- If GUI doesn't start: Try running from Terminal.app
- Audio issues: Check Audio MIDI Setup in System Preferences
- OSC not working: Verify firewall settings and target application

📖 For detailed instructions, see:
- README.md (complete documentation)
- GUI_LAUNCH_INSTRUCTIONS.md (GUI-specific help)

🌐 Support: Check the GitHub repository for issues and updates
EOF

# Create launcher script for Terminal users
cat > "$DIST_DIR/Launch from Terminal.command" << 'EOF'
#!/bin/bash
cd "$(dirname "$0")"
open "CV to OSC Converter.app"
EOF

chmod +x "$DIST_DIR/Launch from Terminal.command"

# Create version info
echo "Version: 1.0.0 (f49a810)" > "$DIST_DIR/VERSION.txt"
echo "Build Date: $(date)" >> "$DIST_DIR/VERSION.txt"
echo "Platform: macOS" >> "$DIST_DIR/VERSION.txt"

echo ""
echo "✅ Distribution package created successfully!"
echo "📁 Location: $DIST_DIR"
echo ""
echo "📦 Contents:"
ls -la "$DIST_DIR"
echo ""
echo "🚀 To test: open '$DIST_DIR/CV to OSC Converter.app'"
