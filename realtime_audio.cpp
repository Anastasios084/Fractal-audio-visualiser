#include <iostream>
#include <portaudio.h>

// Define some constants for audio input
#define SAMPLE_RATE 44100
#define NUM_CHANNELS 1  // Mono audio
#define FRAMES_PER_BUFFER 256

// This function will be used to capture audio from the microphone
void getNextAudioFrame(size_t frameSampleCount, float* data) {
    PaError err;

    // Initialize PortAudio
    err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio initialization error: " << Pa_GetErrorText(err) << std::endl;
        return;
    }

    // Open input stream
    PaStream *stream;
    err = Pa_OpenDefaultStream(&stream,
                               NUM_CHANNELS,  // Input channels (microphone)
                               0,             // No output channels
                               paFloat32,     // Sample format
                               SAMPLE_RATE,   // Sample rate
                               FRAMES_PER_BUFFER,  // Frames per buffer
                               nullptr,       // No callback, use blocking read
                               nullptr);      // No user data

    if (err != paNoError) {
        std::cerr << "PortAudio open stream error: " << Pa_GetErrorText(err) << std::endl;
        Pa_Terminate();
        return;
    }

    // Start the stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio start stream error: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(stream);
        Pa_Terminate();
        return;
    }

    // Read audio frames
    size_t totalSamples = frameSampleCount * NUM_CHANNELS;  // Total number of samples to read
    err = Pa_ReadStream(stream, data, frameSampleCount);

    if (err != paNoError) {
        std::cerr << "PortAudio read stream error: " << Pa_GetErrorText(err) << std::endl;
    }

    // Stop and close the stream
    Pa_StopStream(stream);
    Pa_CloseStream(stream);

    // Terminate PortAudio
    Pa_Terminate();
}

int main() {
    const size_t frameSampleCount = 1024;
    float audioData[frameSampleCount];

    getNextAudioFrame(frameSampleCount, audioData);

    

    return 0;
}
