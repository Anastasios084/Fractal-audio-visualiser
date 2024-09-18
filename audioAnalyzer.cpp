#include "audioAnalyzer.h"
audioAnalyzer::~audioAnalyzer(){
    // Free allocated resources used for FFT calculation
    fftw_destroy_plan(this->spectroData->p);
    fftw_free(this->spectroData->in);
    fftw_free(this->spectroData->out);
    free(this->spectroData);
}


float bpmDetection(streamCallbackData* data, const void *inputBuffer){
    const float *in = (const float *)inputBuffer;
    // Copy audio data into the input vector for Aubio processing
    for (size_t j = 0; j < FRAMES_PER_BUFFER; ++j) {
        data->in_vec->data[j] = in[j];
    }

    aubio_tempo_do(data->tempo, data->in_vec, data->tempo_out);
    if (aubio_tempo_get_last(data->tempo) != 0) {
        float bpm = aubio_tempo_get_bpm(data->tempo);
        data->bpm_sum += bpm;
        data->bpm_count++;
    }
    //cout << data->bpm_sum << " - " << data->bpm_count;
    return data->bpm_sum/data->bpm_count;
}

int audioAnalyzer::checkErr(PaError err){
    if (err != paNoError) {
        printf("PortAudio error: %s\n", Pa_GetErrorText(err));
        return 1;
    }
    return 0;
}

//I haven't managed to pass this as a callback function
// PortAudio stream callback function. Will be called after every
// `FRAMES_PER_BUFFER` audio samples PortAudio captures. Used to process the
// resulting audio sample.
int streamCallback(
    const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags,
    void* userData
) {
    streamCallbackData* self = static_cast<streamCallbackData*>(userData);
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
            
        callbackData->freq = freq;

        if (freq > 15.0 && i < 10) {// VERY meh implementation
            callbackData->lowBeat = true;
            if(callbackData->maxLowBeat < freq){
                callbackData->maxLowBeat = freq;
            }
            cout << "LOW BEAT DETECTED - " << i << endl;
            break;
            // printf("-%f - %f/n",proportion, freq);
        }else if (freq > 25.0 && i > 80) {
            callbackData->highBeat = true;
            if(callbackData->maxHighBeat < freq){
                callbackData->maxHighBeat = freq;
            }
            cout << "HIGH BEAT DETECTED - " << i << endl;
            break;
        } 
    }

    // Display the buffered changes to stdout in the terminal
    // fflush(stdout);
    callbackData->current_bpm = bpmDetection(callbackData, inputBuffer);
    return 0;
}

audioAnalyzer::audioAnalyzer(){

}

int audioAnalyzer::init(){
     // Initialize PortAudio
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
    spectroData->current_bpm = 120.0;
    spectroData->freq = 0.0f;
    spectroData->lowBeat = false;
    spectroData->highBeat = false;
    PaError err;
    err = Pa_Initialize();
    checkErr(err);

    // Allocate and define the callback data used to calculate/display the spectrogram
    this->spectroData->in = (double*)malloc(sizeof(double) * FRAMES_PER_BUFFER);
    this->spectroData->out = (double*)malloc(sizeof(double) * FRAMES_PER_BUFFER);
    if (this->spectroData->in == NULL || this->spectroData->out == NULL) {
        printf("Could not allocate spectro data\n");
        return 0;
    }

    this->spectroData->p = fftw_plan_r2r_1d(
        FRAMES_PER_BUFFER, this->spectroData->in, this->spectroData->out,
        FFTW_R2HC, FFTW_ESTIMATE
    );
    double sampleRatio = FRAMES_PER_BUFFER / SAMPLE_RATE;
    this->spectroData->startIndex = std::ceil(sampleRatio * SPECTRO_FREQ_START);
    this->spectroData->spectroSize = min(
        std::ceil(sampleRatio * SPECTRO_FREQ_END),
        FRAMES_PER_BUFFER / 2.0
    ) - this->spectroData->startIndex;

    // Get and display the number of audio devices accessible to PortAudio
    int numDevices = Pa_GetDeviceCount();
    printf("Number of devices: %d\n", numDevices);

    if (numDevices < 0) {
        printf("Error getting device count.\n");
        return 0;
    } else if (numDevices == 0) {
        printf("There are no available audio devices on this machine.\n");
        return 0;
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
    return 1;
}

int audioAnalyzer::startSession(int seconds, int device){
    PaError err = Pa_Initialize();

    // Use device 0 (for a programmatic solution for choosing a device,
    // `numDevices - 1` is typically the 'default' device

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
        this->spectroData
    );
    checkErr(err);

    // Begin capturing audio
    err = Pa_StartStream(stream);
    checkErr(err);

    // Wait 30 seconds (PortAudio will continue to capture audio)
    Pa_Sleep(seconds * 1000);

    // Stop capturing audio
    err = Pa_StopStream(stream);
    checkErr(err);

    // Close the PortAudio stream
    err = Pa_CloseStream(stream);
    checkErr(err);

    // Terminate PortAudio
    err = Pa_Terminate();
    checkErr(err);
    fftw_destroy_plan(this->spectroData->p);
    fftw_free(this->spectroData->in);
    fftw_free(this->spectroData->out);
    free(this->spectroData);
    return 1;
}


// Returns the lowest of the two given numbers
inline float audioAnalyzer::min(float a, float b) {
    return a < b ? a : b;
}

float audioAnalyzer::getCurrentBPM(){
    return this->spectroData->current_bpm;
}
float audioAnalyzer::getCurrentFrequency(){
    return this->spectroData->freq;
}
bool audioAnalyzer::lowBeat(){
    return this->spectroData->lowBeat;
}
bool audioAnalyzer::highBeat(){
    return this->spectroData->highBeat;
}

float audioAnalyzer::maxLowBeat(){
    return this->spectroData->maxLowBeat;
}
float audioAnalyzer::maxHighBeat(){
    return this->spectroData->maxHighBeat;
}


void audioAnalyzer::setLowBeat(bool){
    this->spectroData->lowBeat = false;
}
void audioAnalyzer::setHighBeat(bool){
    this->spectroData->highBeat = false;
}

