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
} streamCallbackData;


float bpmDetection(streamCallbackData* data, const void *inputBuffer){
    const float *in = (const float *)inputBuffer;
    // cout << "PAPI" << endl;
    // Copy audio data into the input vector for Aubio processing
    for (size_t j = 0; j < FRAMES_PER_BUFFER; ++j) {
        data->in_vec->data[j] = in[j];
        // cout << data->in_vec->data[j] << endl;
    }
    // cout << "PAPI" << endl;

    aubio_tempo_do(data->tempo, data->in_vec, data->tempo_out);
    if (aubio_tempo_get_last(data->tempo) != 0) {
        float bpm = aubio_tempo_get_bpm(data->tempo);
        data->bpm_sum += bpm;
        data->bpm_count++;
    }
    // cout << "PAPI" << endl;

    return data->bpm_sum/data->bpm_count;
}
// Callback data, persisted between calls. Allows us to access the data it
// contains from within the callback function.
static streamCallbackData* spectroData;

// Checks the return value of a PortAudio function. Logs the message and exits
// if there was an error
static void checkErr(PaError err) {
    if (err != paNoError) {
        printf("PortAudio error: %s\n", Pa_GetErrorText(err));
        exit(EXIT_FAILURE);
    }
}

// Returns the lowest of the two given numbers
static inline float min(float a, float b) {
    return a < b ? a : b;
}

// PortAudio stream callback function. Will be called after every
// `FRAMES_PER_BUFFER` audio samples PortAudio captures. Used to process the
// resulting audio sample.
static int streamCallback(
    const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags,
    void* userData
) {
    // Cast our input buffer to a float pointer (since our sample format is `paFloat32`)
    float* in = (float*)inputBuffer;

    // We will not be modifying the output buffer. This line is a no-op.
    (void)outputBuffer;
    streamCallbackData* callbackData = (streamCallbackData*)userData;
    // Set our spectrogram size in the terminal to 100 characters, and move the
    // cursor to the beginning of the line
    int dispSize = 100;
    printf("\r");

    // Copy audio sample to FFTW's input buffer
    for (unsigned long i = 0; i < framesPerBuffer; i++) {
        callbackData->in[i] = in[i * NUM_CHANNELS];
    }
    // Perform FFT on callbackData->in (results will be stored in callbackData->out)
    fftw_execute(callbackData->p);
    // cout << callbackData->p << endl;

    // Draw the spectrogram
    for (int i = 0; i < dispSize; i++) {
        // Sample frequency data logarithmically
        double proportion = std::pow(i / (double)dispSize, 1);
        double freq = callbackData->out[(int)(callbackData->startIndex
            + proportion * callbackData->spectroSize/10)];
        // cout << callbackData->spectroSize << endl;
        // cout << callbackData->out[];
        // cout << proportion << endl;
        // Display full block characters with heights based on frequency intensity
        // if (freq < 0.125) {
        //     printf("▁");
        //     // printf("-%f - %f/n",proportion, freq);
        // } else if (freq < 0.25) {
        //     printf("▂");
        // } else if (freq < 0.375) {
        //     printf("▃");
        // } else if (freq < 0.5) {
        //     printf("▄");
        // } else if (freq < 0.625) {
        //     printf("▅");
        // } else if (freq < 0.75) {
        //     printf("▆");
        // } else if (freq < 0.875) {
        //     printf("▇");
        // } else {
        //     printf("█");
        // }
        
        if (freq > 15.0 && i < 10) {
            
            cout << "LOW BEAT DETECTED - " << i << endl;
            break;
            // printf("-%f - %f/n",proportion, freq);
        }else if (freq > 15.0 && i > 80) {
            cout << "HIGH BEAT DETECTED - " << i << endl;
            break;
        } 
    }

    // Display the buffered changes to stdout in the terminal
    // fflush(stdout);
    cout << bpmDetection(spectroData, inputBuffer);
    return 0;
}

int main() {
    // Initialize Aubio structures
    uint_t win_s = 1024; // Window size
    uint_t hop_s = 512;  // Hop size
    // streamCallbackData analysisData;
    spectroData = (streamCallbackData*)malloc(sizeof(streamCallbackData));


    spectroData->tempo = new_aubio_tempo("default", win_s, hop_s, SAMPLE_RATE);
    spectroData->pitch = new_aubio_pitch("default", win_s, hop_s, SAMPLE_RATE);
    spectroData->filterbank = new_aubio_filterbank(40, win_s);
    spectroData->onset = new_aubio_onset("default", win_s, hop_s, SAMPLE_RATE);
    spectroData->pvoc = new_aubio_pvoc(win_s, hop_s); // Initialize pvoc
    spectroData->in_vec = new_fvec(hop_s);
    spectroData->fftgrain = new_cvec(win_s);          // Initialize fftgrain
    spectroData->tempo_out = new_fvec(1);
    spectroData->pitch_out = new_fvec(1);
    spectroData->filterbank_out = new_fvec(aubio_filterbank_get_power(spectroData->filterbank));
    spectroData->bpm_sum = 0.0;
    spectroData->bpm_count = 0;
    spectroData->brightness_sum = 0.0;
    spectroData->brightness_count = 0;
    // Initialize PortAudio
    PaError err;
    err = Pa_Initialize();
    checkErr(err);

    // Allocate and define the callback data used to calculate/display the spectrogram
    spectroData->in = (double*)malloc(sizeof(double) * FRAMES_PER_BUFFER);
    spectroData->out = (double*)malloc(sizeof(double) * FRAMES_PER_BUFFER);
    if (spectroData->in == NULL || spectroData->out == NULL) {
        printf("Could not allocate spectro data\n");
        exit(EXIT_FAILURE);
    }
    spectroData->p = fftw_plan_r2r_1d(
        FRAMES_PER_BUFFER, spectroData->in, spectroData->out,
        FFTW_R2HC, FFTW_ESTIMATE
    );
    double sampleRatio = FRAMES_PER_BUFFER / SAMPLE_RATE;
    spectroData->startIndex = std::ceil(sampleRatio * SPECTRO_FREQ_START);
    spectroData->spectroSize = min(
        std::ceil(sampleRatio * SPECTRO_FREQ_END),
        FRAMES_PER_BUFFER / 2.0
    ) - spectroData->startIndex;

    // Get and display the number of audio devices accessible to PortAudio
    int numDevices = Pa_GetDeviceCount();
    printf("Number of devices: %d\n", numDevices);

    if (numDevices < 0) {
        printf("Error getting device count.\n");
        exit(EXIT_FAILURE);
    } else if (numDevices == 0) {
        printf("There are no available audio devices on this machine.\n");
        exit(EXIT_SUCCESS);
    }

    // Display audio device information for each device accessible to PortAudio
    const PaDeviceInfo* deviceInfo;
    for (int i = 0; i < numDevices; i++) {
        deviceInfo = Pa_GetDeviceInfo(i);
        printf("Device %d:\n", i);
        printf("  name: %s\n", deviceInfo->name);
        printf("  maxInputChannels: %d\n", deviceInfo->maxInputChannels);
        printf("  maxOutputChannels: %d\n", deviceInfo->maxOutputChannels);
        printf("  defaultSampleRate: %f\n", deviceInfo->defaultSampleRate);
    }

    // Use device 0 (for a programmatic solution for choosing a device,
    // `numDevices - 1` is typically the 'default' device
    int device = 7;

    // Define stream capture specifications
    PaStreamParameters inputParameters;
    memset(&inputParameters, 0, sizeof(inputParameters));
    inputParameters.channelCount = NUM_CHANNELS;
    inputParameters.device = device;
    inputParameters.hostApiSpecificStreamInfo = NULL;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(device)->defaultLowInputLatency;

    // Open the PortAudio stream
    PaStream* stream;
    err = Pa_OpenStream(
        &stream,
        &inputParameters,
        NULL,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paNoFlag,
        streamCallback,
        spectroData
    );
    checkErr(err);

    // Begin capturing audio
    err = Pa_StartStream(stream);
    checkErr(err);

    // Wait 30 seconds (PortAudio will continue to capture audio)
    Pa_Sleep(30 * 1000);

    // Stop capturing audio
    err = Pa_StopStream(stream);
    checkErr(err);

    // Close the PortAudio stream
    err = Pa_CloseStream(stream);
    checkErr(err);

    // Terminate PortAudio
    err = Pa_Terminate();
    checkErr(err);

    // Free allocated resources used for FFT calculation
    fftw_destroy_plan(spectroData->p);
    fftw_free(spectroData->in);
    fftw_free(spectroData->out);
    free(spectroData);

    printf("\n");

    return EXIT_SUCCESS;
}