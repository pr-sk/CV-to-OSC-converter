#!/bin/bash

echo "Testing microphone permission handling in professional_osc_mixer app..."
echo ""

# Check if the app binary exists
if [ ! -f "build/professional_osc_mixer.app/Contents/MacOS/professional_osc_mixer" ]; then
    echo "Error: Application not found. Please build the app first."
    exit 1
fi

# Check current microphone permission status
echo "Checking microphone permission status..."
osascript -e 'try
    set microphoneStatus to do shell script "echo $(defaults read com.apple.TCC.db | grep professional_osc_mixer)"
on error
    set microphoneStatus to "Not found in TCC database"
end try
return microphoneStatus'

echo ""
echo "Instructions for testing:"
echo "1. Open the professional_osc_mixer app"
echo "2. Click on any channel's INPUT button"
echo "3. Select a microphone from the list"
echo "4. Click 'Connect'"
echo ""
echo "Expected behavior:"
echo "- If microphone permission is not granted, you should see a permission dialog"
echo "- The app should NOT hang or freeze"
echo "- You can grant permission or cancel without issues"
echo ""
echo "To reset microphone permissions for testing:"
echo "tccutil reset Microphone com.yourcompany.professional_osc_mixer"
