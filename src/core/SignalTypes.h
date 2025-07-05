#pragma once

// Signal type detection - shared between CVReader and CVWriter
enum class SignalType {
    UNKNOWN,
    CV_SIGNAL,     // Control voltage - slow changing DC signals
    AUDIO_SIGNAL,  // Audio - fast changing AC signals
    AUTO_DETECT    // Automatically determine based on signal characteristics
};

struct SignalAnalysis {
    float dcComponent;        // Average DC level
    float acComponent;        // RMS of AC component  
    float peakToPeak;        // Peak-to-peak variation
    float changeRate;        // Rate of signal change
    int consecutiveStable;   // Consecutive stable readings
    SignalType detectedType;
    float confidence;        // 0.0 to 1.0 confidence in detection
};

// Output signal processing modes
enum class OutputMode {
    DC_OUTPUT,      // Direct DC voltage output for CV
    PWM_OUTPUT,     // PWM modulation for audio outputs
    AUDIO_OUTPUT,   // Standard audio signal output
    AUTO_DETECT     // Automatically determine based on device type
};
