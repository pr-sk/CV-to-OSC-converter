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
#include "PluginProcessor.h"

//==============================================================================
/**
 * CV to OSC Converter Plugin Editor
 * Provides a graphical user interface for the plugin
 */
class CVToOSCEditor : public juce::AudioProcessorEditor
{
public:
    CVToOSCEditor(CVToOSCProcessor*);
    ~CVToOSCEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Reference to processor
    CVToOSCProcessor* audioProcessor;

    // GUI components
    juce::Label titleLabel;
    
    // OSC Configuration
    juce::Label oscHostLabel;
    juce::TextEditor oscHostEditor;
    
    juce::Label oscPortLabel;
    juce::Slider oscPortSlider;
    
    // Audio Processing Controls
    juce::Label gainLabel;
    juce::Slider gainSlider;
    
    juce::Label thresholdLabel;
    juce::Slider thresholdSlider;
    
    juce::Label smoothingLabel;
    juce::Slider smoothingSlider;
    
    // Status indicators
    juce::Label statusLabel;
    juce::TextButton connectButton;
    
    // Visualizations
    juce::Label channelLabels[8];
    std::array<std::unique_ptr<juce::ProgressBar>, 8> channelMeters;
    
    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscPortAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> smoothingAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> oscHostAttachment;
    
    // Timer for updates
    class UpdateTimer : public juce::Timer
    {
    public:
        UpdateTimer(CVToOSCEditor& e) : editor(e) {}
        void timerCallback() override { editor.updateMeters(); }
    private:
        CVToOSCEditor& editor;
    };
    
    UpdateTimer updateTimer;
    
    // Helper methods
    void setupComponent(juce::Component& component, const juce::String& tooltip = {});
    void setupSlider(juce::Slider& slider, double min, double max, double defaultValue, 
                     double interval = 0.0, const juce::String& suffix = {});
    void updateMeters();
    void updateConnectionStatus();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CVToOSCEditor)
};
