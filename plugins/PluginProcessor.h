#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <memory>
#include <atomic>
#include "CVReader.h"
#include "CVWriter.h"
#include "OSCSender.h"
#include "OSCReceiver.h"
#include "OSCFormatManager.h"
#include "Config.h"

//==============================================================================
/**
 * CV to OSC Converter Plugin Processor
 * Handles audio processing and CV to OSC conversion in real-time
 */
class CVToOSCProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    CVToOSCProcessor();
    ~CVToOSCProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
   #endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // Plugin-specific parameters
    enum ParameterIndex
    {
        OSC_HOST = 0,
        OSC_PORT,
        GAIN,
        THRESHOLD,
        SMOOTHING,
        NUM_PARAMETERS
    };

    // Access to parameters
    juce::AudioParameterString* getOSCHostParameter() { return oscHostParam; }
    juce::AudioParameterInt* getOSCPortParameter() { return oscPortParam; }
    juce::AudioParameterFloat* getGainParameter() { return gainParam; }
    juce::AudioParameterFloat* getThresholdParameter() { return thresholdParam; }
    juce::AudioParameterFloat* getSmoothingParameter() { return smoothingParam; }
    
    // Parameter tree access
    juce::AudioProcessorValueTreeState& getParameterTree() { return *parameterTree; }

private:
    //==============================================================================
    // Core components
    std::unique_ptr<CVReader> cvReader_;
    std::unique_ptr<CVWriter> cvWriter_;
    std::unique_ptr<OSCSender> oscSender_;
    std::unique_ptr<OSCReceiver> oscReceiver_;
    std::unique_ptr<OSCFormatManager> formatManager_;
    std::unique_ptr<Config> config_;

    // Parameter tree
    std::unique_ptr<juce::AudioProcessorValueTreeState> parameterTree;
    
    // Plugin parameters
    juce::AudioParameterString* oscHostParam;
    juce::AudioParameterInt* oscPortParam;
    juce::AudioParameterFloat* gainParam;
    juce::AudioParameterFloat* thresholdParam;
    juce::AudioParameterFloat* smoothingParam;

    // Audio processing
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    
    // CV processing buffers
    std::vector<float> cvBuffer_;
    std::vector<float> smoothedValues_;
    
    // Timing and sync
    int sampleCounter = 0;
    int oscUpdateRate = 100; // Updates per second
    int samplesPerOSCUpdate = 441; // Will be calculated based on sample rate
    
    // State
    bool isInitialized = false;
    juce::String lastOSCHost = "127.0.0.1";
    int lastOSCPort = 9000;

    //==============================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void initializeComponents();
    void updateOSCConnection();
    void processCVToOSC(const juce::AudioBuffer<float>& buffer);
    void processOSCToCV(juce::AudioBuffer<float>& buffer);
    void updateParameters();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CVToOSCProcessor)
};
