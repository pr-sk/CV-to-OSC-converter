#!/bin/bash

echo "🚀 Monitoring automatic channel activation..."
echo "Channels will auto-start when input device is added"
echo "Channels will auto-stop when last input device is removed"
echo ""
echo "Press Ctrl+C to stop"
echo ""

while true; do
    clear
    echo "🚀 AUTO-ACTIVATION MONITOR"
    echo "=========================="
    echo ""
    
    # Look for device addition/removal
    echo "📱 Device Changes:"
    log show --predicate 'process == "professional_osc_mixer"' --last 20s 2>/dev/null | \
    grep -E "(Added input device|Removed input device|Auto-starting|Auto-stopping)" | \
    tail -10
    
    echo ""
    echo "🎤 Audio Stream Creation:"
    log show --predicate 'process == "professional_osc_mixer"' --last 20s 2>/dev/null | \
    grep -E "(Creating audio|Successfully created|AudioDeviceIntegration)" | \
    tail -5
    
    echo ""
    echo "📊 Channel States:"
    log show --predicate 'process == "professional_osc_mixer"' --last 10s 2>/dev/null | \
    grep -E "(Channel [0-9]+ started|Channel [0-9]+ stopped)" | \
    tail -5
    
    echo ""
    echo "💚 Active Channels (Processing Audio):"
    log show --predicate 'process == "professional_osc_mixer"' --last 5s 2>/dev/null | \
    grep -E "Channel [0-9]+: Input.*→.*Output" | \
    tail -3
    
    sleep 2
done
