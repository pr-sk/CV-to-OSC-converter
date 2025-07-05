#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <portaudio.h>
#include "src/osc/OSCSender.h"

// Simple audio callback to test input
static int audioCallback(const void *inputBuffer, void *outputBuffer,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo* timeInfo,
                        PaStreamCallbackFlags statusFlags,
                        void *userData) {
    const float *in = (const float*)inputBuffer;
    OSCSender* sender = (OSCSender*)userData;
    
    if (in != nullptr) {
        // Calculate RMS of input signal
        float rms = 0.0f;
        for (unsigned long i = 0; i < framesPerBuffer; i++) {
            rms += in[i] * in[i];
        }
        rms = sqrt(rms / framesPerBuffer);
        
        // Convert to CV (0-10V range)
        float cv = rms * 10.0f;
        
        // Send OSC message
        if (cv > 0.01f) { // Only send if there's signal
            sender->sendFloat("/test/cv", cv);
            std::cout << "Audio RMS: " << rms << " -> CV: " << cv << "V" << std::endl;
        }
    }
    
    return paContinue;
}

int main() {
    try {
        std::cout << "=== Simple Audio Input Test ===" << std::endl;
        
        // Initialize PortAudio
        PaError err = Pa_Initialize();
        if (err != paNoError) {
            std::cerr << "Failed to initialize PortAudio: " << Pa_GetErrorText(err) << std::endl;
            return 1;
        }
        
        // Print available devices
        int numDevices = Pa_GetDeviceCount();
        std::cout << "\nAvailable audio devices:" << std::endl;
        for (int i = 0; i < numDevices; i++) {
            const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
            if (info->maxInputChannels > 0) {
                std::cout << "  [" << i << "] " << info->name 
                         << " (inputs: " << info->maxInputChannels << ")" << std::endl;
            }
        }
        
        // Get default input device
        PaDeviceIndex defaultInput = Pa_GetDefaultInputDevice();
        if (defaultInput == paNoDevice) {
            std::cerr << "No default input device found!" << std::endl;
            Pa_Terminate();
            return 1;
        }
        
        const PaDeviceInfo* inputInfo = Pa_GetDeviceInfo(defaultInput);
        std::cout << "\nUsing default input: " << inputInfo->name << std::endl;
        
        // Create OSC sender
        OSCSender sender("127.0.0.1", "9000");
        std::cout << "OSC sender configured for 127.0.0.1:9000" << std::endl;
        
        // Configure stream parameters
        PaStreamParameters inputParams;
        inputParams.device = defaultInput;
        inputParams.channelCount = 1;
        inputParams.sampleFormat = paFloat32;
        inputParams.suggestedLatency = inputInfo->defaultLowInputLatency;
        inputParams.hostApiSpecificStreamInfo = nullptr;
        
        // Open stream
        PaStream* stream;
        err = Pa_OpenStream(&stream,
                           &inputParams,
                           nullptr, // no output
                           44100,   // sample rate
                           256,     // frames per buffer
                           paClipOff,
                           audioCallback,
                           &sender);
        
        if (err != paNoError) {
            std::cerr << "Failed to open stream: " << Pa_GetErrorText(err) << std::endl;
            Pa_Terminate();
            return 1;
        }
        
        // Start stream
        err = Pa_StartStream(stream);
        if (err != paNoError) {
            std::cerr << "Failed to start stream: " << Pa_GetErrorText(err) << std::endl;
            Pa_CloseStream(stream);
            Pa_Terminate();
            return 1;
        }
        
        std::cout << "\n✅ Audio input test is running!" << std::endl;
        std::cout << "Make some noise into your microphone..." << std::endl;
        std::cout << "OSC messages will be sent to 127.0.0.1:9000 with path /test/cv" << std::endl;
        std::cout << "\nPress Enter to stop..." << std::endl;
        
        // Wait for user input
        std::cin.get();
        
        // Stop and close stream
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
        
        // Terminate PortAudio
        Pa_Terminate();
        
        std::cout << "Test completed!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
