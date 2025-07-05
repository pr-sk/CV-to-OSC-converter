#include <gtest/gtest.h>
#include "../plugins/PluginProcessor.h"

// Mock audio buffer for testing
struct MockAudioBuffer {
    int channels, samples;
    float value;
    MockAudioBuffer(int ch, int smp, float val) : channels(ch), samples(smp), value(val) {}
};

// Mock MIDI buffer for testing
struct MockMidiBuffer {
    MockMidiBuffer() {}
};

// Utility to create a mock AudioBuffer filled with a constant value
static MockAudioBuffer makeBuffer(int channels, int samples, float value)
{
    return MockAudioBuffer(channels, samples, value);
}

TEST(CVToOSCProcessorTest, HostUpdateReconnect)
{
    CVToOSCProcessor proc;
    proc.prepareToPlay(44100.0, 512);

    // Update host and attempt to connect sender
    proc.setOSCHost("192.168.0.2");
    proc.connectOSCSender();
    EXPECT_TRUE(proc.isOSCSenderConnected());

    proc.disconnectOSCSender();
    EXPECT_FALSE(proc.isOSCSenderConnected());
}

TEST(CVToOSCProcessorTest, CVToSmoothedValue)
{
    CVToOSCProcessor proc;
    proc.prepareToPlay(44100.0, 512);

    auto buffer = makeBuffer(1, 512, 1.0f); // constant signal with RMS 1
    MockMidiBuffer dummyMidi;

    // Process enough blocks to ensure at least a couple of OSC update intervals
    for (int i = 0; i < 10; ++i)
    {
        proc.processBlock(buffer, dummyMidi);
    }

    // After processing, smoothed value for channel 0 should be > threshold
    EXPECT_GT(proc.getCVValue(0), 0.1f);
}

