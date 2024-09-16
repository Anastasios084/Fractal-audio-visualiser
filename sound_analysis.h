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
#define SAMPLE_FORMAT paInt32 // 16-bit PCM
#define RECORD_DURATION 10        // Duration of the recording in seconds

vector<PcmAudioFrame> getPcmAudioFrames(const string& path, PcmAudioFrameFormat format, size_t frameSampleCount);
int detect_shouldReturnTheBpmAndTheBeat(const string& path, PcmAudioFrameFormat format);