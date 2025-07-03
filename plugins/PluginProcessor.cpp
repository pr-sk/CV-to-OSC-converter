#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
CVToOSCProcessor::CVToOSCProcessor()
     : AudioProcessor(BusesProperties()
                      .withInput("Input", juce::AudioChannelSet::stereo(), true)
                      .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    // Initialize parameters with proper parameter layout
    auto layout = createParameterLayout();
    parameterTree = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, "Parameters", std::move(layout));
    
    // Get parameter pointers
    oscHostParam = dynamic_cast<juce::AudioParameterString*>(parameterTree->getParameter("oscHost"));
    oscPortParam = dynamic_cast<juce::AudioParameterInt*>(parameterTree->getParameter("oscPort"));
    gainParam = dynamic_cast<juce::AudioParameterFloat*>(parameterTree->getParameter("gain"));
    thresholdParam = dynamic_cast<juce::AudioParameterFloat*>(parameterTree->getParameter("threshold"));
    smoothingParam = dynamic_cast<juce::AudioParameterFloat*>(parameterTree->getParameter("smoothing"));
}

juce::AudioProcessorValueTreeState::ParameterLayout CVToOSCProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<juce::AudioParameterString>(
        "oscHost", "OSC Host", "127.0.0.1"));
    
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "oscPort", "OSC Port", 1024, 65535, 9000));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "gain", "Gain", 0.0f, 2.0f, 1.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "threshold", "Threshold", 0.0f, 1.0f, 0.01f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "smoothing", "Smoothing", 0.0f, 1.0f, 0.1f));
    
    return layout;
}

CVToOSCProcessor::~CVToOSCProcessor()
{
}

//==============================================================================
const juce::String CVToOSCProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CVToOSCProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool CVToOSCProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool CVToOSCProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double CVToOSCProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int CVToOSCProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int CVToOSCProcessor::getCurrentProgram()
{
    return 0;
}

void CVToOSCProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String CVToOSCProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void CVToOSCProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void CVToOSCProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;
    
    // Calculate OSC update timing
    samplesPerOSCUpdate = static_cast<int>(sampleRate / oscUpdateRate);
    
    // Initialize buffers
    cvBuffer_.resize(8, 0.0f); // 8 channels of CV
    smoothedValues_.resize(8, 0.0f);
    
    // Initialize components
    initializeComponents();
    
    isInitialized = true;
}

void CVToOSCProcessor::releaseResources()
{
    // Clean up resources
    if (oscSender_) {
        oscSender_.reset();
    }
    if (oscReceiver_) {
        oscReceiver_.reset();
    }
    if (cvReader_) {
        cvReader_.reset();
    }
    if (cvWriter_) {
        cvWriter_.reset();
    }
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool CVToOSCProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Support mono and stereo configurations
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Input and output layout must match
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}
#endif

void CVToOSCProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    
    if (!isInitialized) {
        return;
    }
    
    juce::ScopedNoDenormals noDenormals;
    
    // Update parameters if they have changed
    updateParameters();
    
    // Process CV to OSC conversion
    processCVToOSC(buffer);
    
    // Process OSC to CV conversion (modifies buffer)
    processOSCToCV(buffer);
    
    // Update sample counter
    sampleCounter += buffer.getNumSamples();
}

//==============================================================================
bool CVToOSCProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* CVToOSCProcessor::createEditor()
{
    return new CVToOSCEditor(*this);
}

//==============================================================================
void CVToOSCProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Create XML to store plugin state
    std::unique_ptr<juce::XmlElement> xml(new juce::XmlElement("CVToOSCSettings"));
    
    xml->setAttribute("oscHost", oscHostParam->get());
    xml->setAttribute("oscPort", oscPortParam->get());
    xml->setAttribute("gain", gainParam->get());
    xml->setAttribute("threshold", thresholdParam->get());
    xml->setAttribute("smoothing", smoothingParam->get());
    
    // Save XML to memory block
    copyXmlToBinary(*xml, destData);
}

void CVToOSCProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Restore state from XML
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName("CVToOSCSettings"))
        {
            *oscHostParam = xmlState->getStringAttribute("oscHost", "127.0.0.1");
            *oscPortParam = xmlState->getIntAttribute("oscPort", 9000);
            *gainParam = (float)xmlState->getDoubleAttribute("gain", 1.0);
            *thresholdParam = (float)xmlState->getDoubleAttribute("threshold", 0.01);
            *smoothingParam = (float)xmlState->getDoubleAttribute("smoothing", 0.1);
        }
    }
}

//==============================================================================
// Private methods

void CVToOSCProcessor::initializeComponents()
{
    try {
        // Initialize configuration
        config_ = std::make_unique<Config>();
        
        // Initialize format manager
        formatManager_ = std::make_unique<OSCFormatManager>();
        
        // Initialize OSC sender
        std::string host = oscHostParam->get().toStdString();
        std::string port = std::to_string(oscPortParam->get());
        oscSender_ = std::make_unique<OSCSender>(host, port);
        
        // Initialize OSC receiver
        oscReceiver_ = std::make_unique<OSCReceiver>("8001", formatManager_);
        
        // Set up OSC receiver callback for OSC to CV conversion
        oscReceiver_->setMessageCallback([this](const std::string& address, const std::vector<float>& values) {
            // Parse channel from address and update CV buffer
            if (address.find("/cv/") == 0 && values.size() > 0) {
                std::string channelStr = address.substr(4);
                int channel = std::stoi(channelStr);
                if (channel >= 0 && channel < static_cast<int>(cvBuffer_.size())) {
                    cvBuffer_[channel] = values[0];
                }
            }
        });
        
        oscReceiver_->start();
        
    } catch (const std::exception& e) {
        // Log error but don't crash
        juce::Logger::writeToLog("CV to OSC Plugin initialization error: " + juce::String(e.what()));
    }
}

void CVToOSCProcessor::updateOSCConnection()
{
    if (!oscSender_) return;
    
    juce::String currentHost = oscHostParam->get();
    int currentPort = oscPortParam->get();
    
    if (currentHost != lastOSCHost || currentPort != lastOSCPort) {
        // Reconnect with new settings
        oscSender_.reset();
        oscSender_ = std::make_unique<OSCSender>(
            currentHost.toStdString(), 
            std::to_string(currentPort)
        );
        
        lastOSCHost = currentHost;
        lastOSCPort = currentPort;
    }
}

void CVToOSCProcessor::processCVToOSC(const juce::AudioBuffer<float>& buffer)
{
    if (!oscSender_ || sampleCounter % samplesPerOSCUpdate != 0) {
        return;
    }
    
    const int numChannels = juce::jmin(buffer.getNumChannels(), static_cast<int>(cvBuffer_.size()));
    const float gain = gainParam->get();
    const float threshold = thresholdParam->get();
    const float smoothing = smoothingParam->get();
    
    for (int channel = 0; channel < numChannels; ++channel) {
        const float* channelData = buffer.getReadPointer(channel);
        
        // Calculate RMS value for the block
        float rms = 0.0f;
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            float value = channelData[sample] * gain;
            rms += value * value;
        }
        rms = std::sqrt(rms / buffer.getNumSamples());
        
        // Apply threshold
        if (rms < threshold) {
            rms = 0.0f;
        }
        
        // Apply smoothing
        smoothedValues_[channel] = smoothedValues_[channel] * smoothing + rms * (1.0f - smoothing);
        
        // Send OSC message
        std::string address = "/cv/" + std::to_string(channel);
        try {
            oscSender_->sendFloat(address, smoothedValues_[channel]);
        } catch (const std::exception& e) {
            juce::Logger::writeToLog("OSC send error: " + juce::String(e.what()));
        }
    }
}

void CVToOSCProcessor::processOSCToCV(juce::AudioBuffer<float>& buffer)
{
    // Convert OSC values back to CV (modify output buffer)
    const int numChannels = juce::jmin(buffer.getNumChannels(), static_cast<int>(cvBuffer_.size()));
    
    for (int channel = 0; channel < numChannels; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        float cvValue = cvBuffer_[channel];
        
        // Apply CV value to output buffer
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            channelData[sample] = cvValue;
        }
    }
}

void CVToOSCProcessor::updateParameters()
{
    updateOSCConnection();
}

//==============================================================================
// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CVToOSCProcessor();
}
