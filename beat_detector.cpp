#include <MusicBeatDetector/MusicBeatDetector.h>
#include <portaudio.h>
#include <iostream>
#include <sndfile.h>
#include <alsa/asoundlib.h>
#include <lame/lame.h>

using namespace introlab;
using namespace std;
// Define some constants for audio input
#define SAMPLE_RATE 44100
#define NUM_CHANNELS 1  // Mono audio
#define FRAMES_PER_BUFFER 512
#define NUM_SECONDS 1
#define SAMPLE_FORMAT paInt16 // 16-bit PCM
#define PCM_DEVICE "default"

int recordAudioToFile(unsigned int sampleRate, string filePath){
        snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *params;
    int rc;
    int dir;
    snd_pcm_uframes_t frames = FRAMES_PER_BUFFER;
    short *buffer;
    int bufferSize;

    // unsigned int sampleRate = SAMPLE_RATE;

    // Open PCM device for recording
    rc = snd_pcm_open(&pcm_handle, PCM_DEVICE, SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0) {
        std::cerr << "Unable to open PCM device: " << snd_strerror(rc) << std::endl;
        return 1;
    }

    // Allocate hardware parameters object
    snd_pcm_hw_params_malloc(&params);
    snd_pcm_hw_params_any(pcm_handle, params);
    
    // Set parameters
    snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm_handle, params, NUM_CHANNELS);
    snd_pcm_hw_params_set_rate_near(pcm_handle, params, &sampleRate, &dir);
    snd_pcm_hw_params_set_period_size_near(pcm_handle, params, &frames, &dir);

    // Write parameters
    rc = snd_pcm_hw_params(pcm_handle, params);
    if (rc < 0) {
        std::cerr << "Unable to set PCM parameters: " << snd_strerror(rc) << std::endl;
        snd_pcm_hw_params_free(params);
        snd_pcm_close(pcm_handle);
        return 1;
    }

    // Allocate buffer to hold one period
    snd_pcm_hw_params_get_period_size(params, &frames, &dir);
    bufferSize = frames * NUM_CHANNELS;
    buffer = new short[bufferSize];

    // Initialize LAME
    lame_t lame = lame_init();
    lame_set_in_samplerate(lame, SAMPLE_RATE);
    lame_set_num_channels(lame, NUM_CHANNELS);
    lame_set_VBR(lame, vbr_default);
    lame_init_params(lame);

    // Open file for MP3 output
    FILE *mp3File = fopen(filePath.c_str(), "wb");
    if (!mp3File) {
        std::cerr << "Unable to open output file." << std::endl;
        snd_pcm_hw_params_free(params);
        snd_pcm_close(pcm_handle);
        lame_close(lame);
        delete[] buffer;
        return 1;
    }

    // Buffer for MP3 encoding
    int mp3BufferSize = 1.25 * bufferSize + 7200; // Size estimate for MP3 buffer
    unsigned char *mp3Buffer = new unsigned char[mp3BufferSize];

    // Determine the number of loops for the desired duration
    int loops = SAMPLE_RATE * NUM_SECONDS / frames;

    // Capture audio and encode to MP3
    for (int i = 0; i < loops; ++i) {
        rc = snd_pcm_readi(pcm_handle, buffer, frames);
        if (rc == -EPIPE) {
            std::cerr << "Overrun occurred" << std::endl;
            snd_pcm_prepare(pcm_handle);
        } else if (rc < 0) {
            std::cerr << "Error reading PCM device: " << snd_strerror(rc) << std::endl;
            break;
        } else if (rc != (int)frames) {
            std::cerr << "Short read, read " << rc << " frames" << std::endl;
        }

        // Encode the PCM data to MP3
        int mp3Bytes = lame_encode_buffer_interleaved(lame, buffer, rc, mp3Buffer, mp3BufferSize);
        fwrite(mp3Buffer, 1, mp3Bytes, mp3File);
    }

    // Flush the MP3 buffer
    int mp3Bytes = lame_encode_flush(lame, mp3Buffer, mp3BufferSize);
    fwrite(mp3Buffer, 1, mp3Bytes, mp3File);

    // Clean up
    fclose(mp3File);
    snd_pcm_drain(pcm_handle);
    snd_pcm_hw_params_free(params);
    snd_pcm_close(pcm_handle);
    lame_close(lame);
    delete[] buffer;
    delete[] mp3Buffer;

    std::cout << "Recording completed successfully." << std::endl;
    return 0;
}














vector<PcmAudioFrame> getPcmAudioFrames(const string& path, PcmAudioFrameFormat format, size_t frameSampleCount)
{
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

int main(int argc, char** argv)
{
    std::string filePath = "./Passionfruit.mp3";

    // recordAudioToFile(44100, filePath);

    return detect_shouldReturnTheBpmAndTheBeat(filePath, PcmAudioFrameFormat::Float);
}