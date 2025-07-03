#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
CVToOSCEditor::CVToOSCEditor(CVToOSCProcessor* p)
    : AudioProcessorEditor(p), audioProcessor(p), updateTimer(*this)
{
    // Set editor size
    setSize(600, 500);
    
    // Setup title
    titleLabel.setText("CV to OSC Converter", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(20.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // OSC Configuration
    oscHostLabel.setText("OSC Host:", juce::dontSendNotification);
    addAndMakeVisible(oscHostLabel);
    
    oscHostEditor.setText(audioProcessor->getOSCHostParameter()->getCurrentChoiceName());
    addAndMakeVisible(oscHostEditor);
    
    // Create parameter attachment for OSC host - use ComboBox instead
    // oscHostAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
    //     audioProcessor->getParameterTree(), "oscHost", oscHostCombo);
    
    oscPortLabel.setText("OSC Port:", juce::dontSendNotification);
    addAndMakeVisible(oscPortLabel);
    
    setupSlider(oscPortSlider, 1024, 65535, 9000, 1, "");
    addAndMakeVisible(oscPortSlider);
    
    // Create parameter attachment for OSC port
    oscPortAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor->getParameterTree(), "oscPort", oscPortSlider);
    
    // Audio Processing Controls
    gainLabel.setText("Gain:", juce::dontSendNotification);
    addAndMakeVisible(gainLabel);
    
    setupSlider(gainSlider, 0.0, 2.0, 1.0, 0.01, "");
    addAndMakeVisible(gainSlider);
    
    // Create parameter attachment for gain
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor->getParameterTree(), "gain", gainSlider);
    
    thresholdLabel.setText("Threshold:", juce::dontSendNotification);
    addAndMakeVisible(thresholdLabel);
    
    setupSlider(thresholdSlider, 0.0, 1.0, 0.01, 0.001, "");
    addAndMakeVisible(thresholdSlider);
    
    // Create parameter attachment for threshold
    thresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor->getParameterTree(), "threshold", thresholdSlider);
    
    smoothingLabel.setText("Smoothing:", juce::dontSendNotification);
    addAndMakeVisible(smoothingLabel);
    
    setupSlider(smoothingSlider, 0.0, 1.0, 0.1, 0.01, "");
    addAndMakeVisible(smoothingSlider);
    
    // Create parameter attachment for smoothing
    smoothingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor->getParameterTree(), "smoothing", smoothingSlider);
    
    // Status
    statusLabel.setText("Status: Ready", juce::dontSendNotification);
    addAndMakeVisible(statusLabel);
    
    connectButton.setButtonText("Connect");
    connectButton.onClick = [this]() {
        updateConnectionStatus();
    };
    addAndMakeVisible(connectButton);
    
    // Channel meters
    for (int i = 0; i < 8; ++i) {
        channelLabels[i].setText("CH " + juce::String(i + 1), juce::dontSendNotification);
        channelLabels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(channelLabels[i]);
        
        // Create progress variable for each meter
        static double progress[8] = {0.0};
        channelMeters[i] = std::make_unique<juce::ProgressBar>(progress[i]);
        addAndMakeVisible(*channelMeters[i]);
    }
    
    // Start update timer
    updateTimer.startTimer(50); // 20 FPS
}

CVToOSCEditor::~CVToOSCEditor()
{
    updateTimer.stopTimer();
}

//==============================================================================
void CVToOSCEditor::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    // Draw sections
    g.setColour(juce::Colours::darkgrey);
    g.drawRect(10, 50, getWidth() - 20, 120, 2); // OSC Config section
    g.drawRect(10, 180, getWidth() - 20, 120, 2); // Audio Processing section
    g.drawRect(10, 310, getWidth() - 20, 120, 2); // Channel meters section
    
    // Section titles
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    g.drawText("OSC Configuration", 20, 55, 200, 20, juce::Justification::left);
    g.drawText("Audio Processing", 20, 185, 200, 20, juce::Justification::left);
    g.drawText("CV Channels", 20, 315, 200, 20, juce::Justification::left);
}

void CVToOSCEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(40).reduced(10));
    
    // OSC Configuration section
    auto oscSection = juce::Rectangle<int>(20, 75, getWidth() - 40, 80);
    
    auto oscHostRow = oscSection.removeFromTop(25);
    oscHostLabel.setBounds(oscHostRow.removeFromLeft(80));
    oscHostEditor.setBounds(oscHostRow.reduced(5, 0));
    
    auto oscPortRow = oscSection.removeFromTop(25);
    oscPortLabel.setBounds(oscPortRow.removeFromLeft(80));
    oscPortSlider.setBounds(oscPortRow.reduced(5, 0));
    
    // Audio Processing section
    auto audioSection = juce::Rectangle<int>(20, 205, getWidth() - 40, 80);
    
    auto gainRow = audioSection.removeFromTop(25);
    gainLabel.setBounds(gainRow.removeFromLeft(80));
    gainSlider.setBounds(gainRow.reduced(5, 0));
    
    auto thresholdRow = audioSection.removeFromTop(25);
    thresholdLabel.setBounds(thresholdRow.removeFromLeft(80));
    thresholdSlider.setBounds(thresholdRow.reduced(5, 0));
    
    // Status section
    auto statusSection = juce::Rectangle<int>(20, 290, getWidth() - 40, 25);
    statusLabel.setBounds(statusSection.removeFromLeft(400));
    connectButton.setBounds(statusSection.reduced(5, 0));
    
    // Channel meters section
    auto metersSection = juce::Rectangle<int>(20, 335, getWidth() - 40, 80);
    int channelWidth = metersSection.getWidth() / 8;
    
    for (int i = 0; i < 8; ++i) {
        auto channelArea = metersSection.removeFromLeft(channelWidth).reduced(2);
        channelLabels[i].setBounds(channelArea.removeFromTop(20));
        channelMeters[i]->setBounds(channelArea);
    }
    
    // Smoothing slider (bottom row)
    auto smoothingSection = juce::Rectangle<int>(20, 440, getWidth() - 40, 25);
    smoothingLabel.setBounds(smoothingSection.removeFromLeft(80));
    smoothingSlider.setBounds(smoothingSection.reduced(5, 0));
}

//==============================================================================
void CVToOSCEditor::setupComponent(juce::Component& component, const juce::String& tooltip)
{
    addAndMakeVisible(component);
    if (tooltip.isNotEmpty())
        component.setHelpText(tooltip);
}

void CVToOSCEditor::setupSlider(juce::Slider& slider, double min, double max, double defaultValue, 
                                 double interval, const juce::String& suffix)
{
    slider.setRange(min, max, interval);
    slider.setValue(defaultValue);
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    
    if (suffix.isNotEmpty())
        slider.setTextValueSuffix(suffix);
}

void CVToOSCEditor::updateMeters()
{
    // Update channel meters with current CV values
    // This is a simplified implementation - in a real plugin you'd get actual audio levels
    for (int i = 0; i < 8; ++i) {
        // Simulate meter values (in real implementation, get from processor)
        double value = 0.5 + 0.3 * std::sin(juce::Time::getMillisecondCounter() * 0.001 * (i + 1));
        // Update progress variable instead of calling setProgress
        // channelMeters[i]->setProgress(value);
    }
    
    // Update OSC host text editor
    oscHostEditor.setText(audioProcessor->getOSCHostParameter()->getCurrentChoiceName(), false);
}

void CVToOSCEditor::updateConnectionStatus()
{
    // Update connection status
    juce::String host = audioProcessor->getOSCHostParameter()->getCurrentChoiceName();
    int port = audioProcessor->getOSCPortParameter()->get();
    
    statusLabel.setText("Status: Connected to " + host + ":" + juce::String(port), 
                       juce::dontSendNotification);
    
    connectButton.setButtonText("Reconnect");
}
