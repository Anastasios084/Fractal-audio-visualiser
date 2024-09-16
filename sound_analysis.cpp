#include "./sound_analysis.h"

// Callback function to capture audio data
static int recordCallback(const void* inputBuffer, void* outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void* userData)
{
    // Cast the input buffer to 16-bit PCM format
    auto* audioBuffers = static_cast<std::vector<std::vector<short>>*>(userData);
    const short* in = static_cast<const short*>(inputBuffer);

    // Copy the input buffer data to a vector
    if (in != nullptr) {
        std::vector<short> buffer(in, in + framesPerBuffer*sizeof(short));
        audioBuffers->push_back(buffer);
    }else{
        cout << "NULL AUDIOBUFFER" << endl;
    }

    // Continue recording
    return paContinue;
}

std::vector<std::vector<short>> getRawAudio() {
    std::vector<std::vector<short>> audioBuffers;

    // Initialize PortAudio
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        
    }

    // Set up input parameters
    PaStreamParameters inputParameters;
    inputParameters.device = Pa_GetDefaultInputDevice();
    if (inputParameters.device == paNoDevice) {
        std::cerr << "Error: No default input device." << std::endl;
    }

    inputParameters.channelCount = NUM_CHANNELS;
    inputParameters.sampleFormat = SAMPLE_FORMAT;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = nullptr;

    // Open an audio stream
    PaStream* stream;
    err = Pa_OpenStream(&stream,
                        &inputParameters,
                        nullptr,  // No output parameters (input only)
                        SAMPLE_RATE,
                        FRAMES_PER_BUFFER,
                        paClipOff,  // No clipping
                        recordCallback,
                        &audioBuffers);   // No user data

    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
    }

    // Start the stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
    }

    // Record for 5 seconds
    std::cout << "Recording for " << RECORD_DURATION << " seconds..." << std::endl;
    Pa_Sleep(RECORD_DURATION * 1000);

    // Stop the stream
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
    }

    // Close the stream
    Pa_CloseStream(stream);
    Pa_Terminate();

    // Print the number of buffers recorded
    std::cout << "Recording finished. Number of buffers: " << audioBuffers.size() << std::endl;

    return audioBuffers;
}



vector<PcmAudioFrame> getPcmAudioFrames(const string& path, PcmAudioFrameFormat format, size_t frameSampleCount)
{
    // // std::ifstream file(path, std::ifstream::in);
    // vector<PcmAudioFrame> frames;
    // std::vector<std::vector<short>> input = getRawAudio();
    // for(int i = 0; i < input.size(); ++i)
    // {
    //     PcmAudioFrame frame(format, 1, frameSampleCount);
        
    //     // Ensure we do not overflow the frame's data buffer
    //     size_t copySize = std::min(input[i].size() * sizeof(short), frame.size());
        
    //     // Copy the raw audio data into the frame
    //     memcpy(frame.data(), input[i].data(), copySize);
        
    //     frames.push_back(frame);
    // }
    // cout << "GOT OUT" << endl;

    // return frames;

 std::ifstream file(path, std::ifstream::in);
    vector<PcmAudioFrame> frames;
    
    while (file.good())
    {
        PcmAudioFrame frame(format, 1, frameSampleCount);
        memset(frame.data(), 0, frame.size());
        file.read(reinterpret_cast<char*>(frame.data()), frame.size());
        frames.push_back(frame);
    }

    return frames;
}

int detect_shouldReturnTheBpmAndTheBeat(const string& path, PcmAudioFrameFormat format)
{
    constexpr float SamplingFrequency = 44100;
    constexpr size_t FrameSampleCount = FRAMES_PER_BUFFER;

    vector<PcmAudioFrame> frames = getPcmAudioFrames(path, format, FrameSampleCount);
    MusicBeatDetector musicBeatDetector(SamplingFrequency, FrameSampleCount);

    vector<float> bpms;
    size_t beatCount = 0;
    for (const PcmAudioFrame& frame : frames)
    {
        Beat beat = musicBeatDetector.detect(frame);
        bpms.push_back(beat.bpm);
        if (beat.isBeat)
        {
            beatCount++;
        }
    }

    float bpmMean = accumulate(bpms.begin(), bpms.end(), 0.0) / bpms.size();

    cout << (int)bpmMean << endl;
    return (int)bpmMean;
}
