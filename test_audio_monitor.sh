#!/bin/bash

echo "🎤 Monitoring audio input for professional_osc_mixer..."
echo "Make sure:"
echo "1. The app is running"
echo "2. You've added an audio input device to a channel"
echo "3. The channel is STARTED (green state)"
echo ""
echo "Looking for audio activity logs..."
echo "Press Ctrl+C to stop"
echo ""

while true; do
    # Look for real audio input logs
    log show --predicate 'process == "professional_osc_mixer"' --last 5s 2>/dev/null | \
    grep -E "(Real audio input|Creating real audio stream|Successfully created audio|Started real audio|getInputSample)" | \
    tail -5
    
    # Also check for channel processing logs
    log show --predicate 'process == "professional_osc_mixer"' --last 5s 2>/dev/null | \
    grep -E "(Channel [0-9]+: Input|processChannel)" | \
    tail -5
    
    sleep 2
done
