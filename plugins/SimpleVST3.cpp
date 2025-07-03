// Simplified VST3 Plugin Implementation for CV to OSC Converter
// This is a demonstration/proof-of-concept implementation
// For production use, implement with full JUCE framework

#include <iostream>
#include <memory>
#include <vector>
#include "CVReader.h"
#include "OSCSender.h"

// Simplified VST3 interface
class CVToOSCVST3 {
public:
    CVToOSCVST3() {
        // Initialize core components
        try {
            oscSender_ = std::make_unique<OSCSender>("127.0.0.1", "9000");
            std::cout << "CV to OSC VST3 Plugin initialized" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "VST3 Plugin initialization error: " << e.what() << std::endl;
        }
    }
    
    ~CVToOSCVST3() {
        std::cout << "CV to OSC VST3 Plugin destroyed" << std::endl;
    }
    
    // Process audio block
    void processAudio(float** inputs, float** outputs, int numChannels, int numSamples) {
        // Simple pass-through with CV to OSC conversion
        for (int channel = 0; channel < numChannels; ++channel) {
            float rms = 0.0f;
            
            // Calculate RMS and copy input to output
            for (int sample = 0; sample < numSamples; ++sample) {
                float value = inputs[channel][sample];
                outputs[channel][sample] = value; // Pass-through
                rms += value * value;
            }
            
            rms = std::sqrt(rms / numSamples);
            
            // Send OSC message (throttled)
            if (sampleCounter_ % 441 == 0) { // ~100Hz at 44.1kHz
                try {
                    std::string address = "/cv/" + std::to_string(channel);
                    oscSender_->sendFloat(address, rms);
                } catch (const std::exception& e) {
                    // Silently ignore OSC errors in plugin context
                }
            }
        }
        
        sampleCounter_ += numSamples;
    }
    
    // Set parameter
    void setParameter(int index, float value) {
        switch (index) {
            case 0: // OSC Port
                if (value > 1024 && value < 65535) {
                    oscPort_ = static_cast<int>(value);
                    reconnectOSC();
                }
                break;
            case 1: // Gain
                gain_ = value;
                break;
            case 2: // Threshold  
                threshold_ = value;
                break;
        }
    }
    
    float getParameter(int index) {
        switch (index) {
            case 0: return static_cast<float>(oscPort_);
            case 1: return gain_;
            case 2: return threshold_;
            default: return 0.0f;
        }
    }

private:
    std::unique_ptr<OSCSender> oscSender_;
    int sampleCounter_ = 0;
    int oscPort_ = 9000;
    float gain_ = 1.0f;
    float threshold_ = 0.01f;
    
    void reconnectOSC() {
        try {
            oscSender_.reset();
            oscSender_ = std::make_unique<OSCSender>("127.0.0.1", std::to_string(oscPort_));
        } catch (const std::exception& e) {
            std::cerr << "OSC reconnection failed: " << e.what() << std::endl;
        }
    }
};

// Export functions for plugin host
extern "C" {
    CVToOSCVST3* createPlugin() {
        return new CVToOSCVST3();
    }
    
    void destroyPlugin(CVToOSCVST3* plugin) {
        delete plugin;
    }
    
    void processAudio(CVToOSCVST3* plugin, float** inputs, float** outputs, 
                     int numChannels, int numSamples) {
        if (plugin) {
            plugin->processAudio(inputs, outputs, numChannels, numSamples);
        }
    }
    
    void setParameter(CVToOSCVST3* plugin, int index, float value) {
        if (plugin) {
            plugin->setParameter(index, value);
        }
    }
    
    float getParameter(CVToOSCVST3* plugin, int index) {
        return plugin ? plugin->getParameter(index) : 0.0f;
    }
}
