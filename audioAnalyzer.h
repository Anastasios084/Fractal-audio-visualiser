#ifndef AUDIOANALYZER_H
#define AUDIOANALYZER_H

#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <cmath>
#include <iostream>
#include <portaudio.h> // PortAudio: Used for audio capture
#include <fftw3.h>     // FFTW:      Provides a discrete FFT algorithm to get
#include <aubio/aubio.h>
#include <vector>

                       //            frequency data from captured audio

#define SAMPLE_RATE 44100.0   // How many audio samples to capture every second (44100 Hz is standard)
#define FRAMES_PER_BUFFER 1024 // How many audio samples to send to our callback function for each channel
#define NUM_CHANNELS 1        // Number of audio channels to capture

#define SPECTRO_FREQ_START 20  // Lower bound of the displayed spectrogram (Hz)
#define SPECTRO_FREQ_END 20000 // Upper bound of the displayed spectrogram (Hz)
using namespace std;
// Define our callback data (data that is passed to every callback function call)
#define DETAIL 100

typedef struct {
    double* in;      // Input buffer, will contain our audio sample
    double* out;     // Output buffer, FFTW will write to this based on the input buffer's contents
    fftw_plan p;     // Created by FFTW to facilitate FFT calculation
    int startIndex;  // First index of our FFT output to display in the spectrogram
    int spectroSize; // Number of elements in our FFT output to display from the start index

    aubio_tempo_t *tempo;
    aubio_pitch_t *pitch;
    aubio_filterbank_t *filterbank;
    aubio_onset_t *onset;
    aubio_pvoc_t *pvoc;  // For FFT
    fvec_t *in_vec;
    cvec_t *fftgrain;    // FFT output
    fvec_t *tempo_out;
    fvec_t *pitch_out;
    fvec_t *filterbank_out;

    float bpm_sum;
    int bpm_count;
    float brightness_sum;
    int brightness_count;
    std::vector<float> low_beats;
    std::vector<float> high_beats;

    float current_bpm = 120.0;
    double freq = 0.0f;
    bool lowBeat = false;
    bool highBeat = false;

} streamCallbackData;


class audioAnalyzer{
    private:
        streamCallbackData* spectroData;
        int device;
        // float bpmDetection(streamCallbackData*, const void*);
        int checkErr(PaError);
        inline float min(float, float);
        // int streamCallback(const void*, void*, unsigned long,
        // const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags,void*);
    public:
        audioAnalyzer();
        ~audioAnalyzer();
        int init();
        int startSession(int, int device=7);

        float getCurrentBPM();
        float getCurrentFrequency();
        bool lowBeat();
        bool highBeat();




};

#endif


