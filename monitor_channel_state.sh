#!/bin/bash

echo "🔍 Monitoring professional_osc_mixer channel state and button clicks..."
echo "Press Ctrl+C to stop"
echo ""

while true; do
    # Clear screen for better readability
    clear
    echo "🔍 CHANNEL STATE MONITOR"
    echo "========================"
    echo ""
    
    # Look for button clicks
    echo "📱 Recent Button Clicks:"
    log show --predicate 'process == "professional_osc_mixer"' --last 10s 2>/dev/null | \
    grep -E "(▶️|button clicked|Channel.*started|Channel.*stopped|Starting channel|Stopping channel)" | \
    tail -5
    
    echo ""
    echo "🎤 Audio Stream Activity:"
    log show --predicate 'process == "professional_osc_mixer"' --last 10s 2>/dev/null | \
    grep -E "(Creating audio|Successfully created|Started real audio|Audio stream)" | \
    tail -5
    
    echo ""
    echo "📊 Channel Processing:"
    log show --predicate 'process == "professional_osc_mixer"' --last 5s 2>/dev/null | \
    grep -E "(Channel [0-9]+: Input.*→.*Output|processChannel|getInputSample)" | \
    tail -3
    
    echo ""
    echo "❌ Errors:"
    log show --predicate 'process == "professional_osc_mixer"' --last 10s 2>/dev/null | \
    grep -E "(Error|Failed|❌)" | \
    tail -3
    
    sleep 2
done
