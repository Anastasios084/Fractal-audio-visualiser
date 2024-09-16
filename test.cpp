// #include <iostream>
// #include <aubio/aubio.h>
// #include <portaudio.h>
// #include <vector>

// #define SAMPLE_RATE 44100
// #define FRAMES_PER_BUFFER 128
// #define NUM_SECONDS 5

// std::vector<float> audioData;

// // Callback function to capture audio input
// int audioCallback(const void *inputBuffer, void *outputBuffer,
//                   unsigned long framesPerBuffer,
//                   const PaStreamCallbackTimeInfo* timeInfo,
//                   PaStreamCallbackFlags statusFlags,
//                   void *userData) {
//     // Cast input buffer to float
//     const float *in = (const float*)inputBuffer;

//     // Store the input audio data into the audioData vector
//     for (unsigned long i = 0; i < framesPerBuffer; ++i) {
//         audioData.push_back(in[i]);
//     }

//     return paContinue;
// }

// int main() {
//     // Initialize PortAudio
//     PaError err = Pa_Initialize();
//     if (err != paNoError) {
//         std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
//         return 1;
//     }

//     // Open default audio input stream
//     PaStream *stream;
//     err = Pa_OpenDefaultStream(&stream,
//                                1,          // Input channels
//                                0,          // Output channels
//                                paFloat32,  // Sample format
//                                SAMPLE_RATE,
//                                FRAMES_PER_BUFFER,
//                                audioCallback,
//                                nullptr);
//     if (err != paNoError) {
//         std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
//         Pa_Terminate();
//         return 1;
//     }

//     // Start the audio stream
//     err = Pa_StartStream(stream);
//     if (err != paNoError) {
//         std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
//         Pa_Terminate();
//         return 1;
//     }

//     // Capture audio for NUM_SECONDS
//     std::cout << "Listening for " << NUM_SECONDS << " seconds..." << std::endl;
//     Pa_Sleep(NUM_SECONDS * 1000);

//     // Stop the audio stream
//     err = Pa_StopStream(stream);
//     if (err != paNoError) {
//         std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
//         Pa_Terminate();
//         return 1;
//     }

//     // Close the audio stream
//     Pa_CloseStream(stream);
//     Pa_Terminate();

//     // Initialize Aubio for BPM detection
//     uint_t win_s = 1024; // Window size
//     uint_t hop_s = 512;  // Hop size
//     aubio_tempo_t *tempo = new_aubio_tempo("default", win_s, hop_s, SAMPLE_RATE);
//     fvec_t *in_vec = new_fvec(hop_s); // Input buffer
//     fvec_t *out = new_fvec(2);
//     uint_t total_frames = audioData.size();
//     float bpm = 0.0;
//     int bpm_count = 0;

//     // Process the recorded audio and calculate BPM
//     for (size_t i = 0; i < total_frames; i += hop_s) {
//         // Copy audio data into the input vector
//         for (size_t j = 0; j < hop_s && i + j < total_frames; ++j) {
//             in_vec->data[j] = audioData[i + j];
//         }

//         // Process with Aubio
//         aubio_tempo_do(tempo, in_vec, out);

//         // Check if a beat is detected
//         if (aubio_tempo_get_last(tempo) != 0) {
//             bpm += aubio_tempo_get_bpm(tempo);
//             bpm_count++;
//         }
//     }

//     // Compute the average BPM
//     if (bpm_count > 0) {
//         bpm /= bpm_count;
//         std::cout << "Estimated BPM: " << bpm << std::endl;
//     } else {
//         std::cout << "No beats detected." << std::endl;
//     }

//     // Clean up
//     del_aubio_tempo(tempo);
//     del_fvec(in_vec);

//     return 0;
// }
#include <iostream>
#include <aubio/aubio.h>
#include <portaudio.h>
#include <vector>

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 1024

// Structure to hold the Aubio objects and analysis data
struct AudioAnalysisData {
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
};

// Callback function to capture audio input and perform real-time analysis
int audioCallback(const void *inputBuffer, void *outputBuffer,
                  unsigned long framesPerBuffer,
                  const PaStreamCallbackTimeInfo *timeInfo,
                  PaStreamCallbackFlags statusFlags,
                  void *userData) {
    // Cast input buffer to float
    const float *in = (const float *)inputBuffer;
    AudioAnalysisData *data = (AudioAnalysisData *)userData;

    // Copy audio data into the input vector for Aubio processing
    for (size_t j = 0; j < framesPerBuffer; ++j) {
        data->in_vec->data[j] = in[j];
    }

    // Process with Aubio for BPM detection
    aubio_tempo_do(data->tempo, data->in_vec, data->tempo_out);
    if (aubio_tempo_get_last(data->tempo) != 0) {
        float bpm = aubio_tempo_get_bpm(data->tempo);
        data->bpm_sum += bpm;
        data->bpm_count++;
    }

    // Process for brilliance (spectral centroid)
    // Perform FFT on the input signal
    aubio_pvoc_do(data->pvoc, data->in_vec, data->fftgrain);

    // Use the FFT output to compute the filterbank
    aubio_filterbank_do(data->filterbank, data->fftgrain, data->filterbank_out);
    float brightness = 0.0;
    // Get the number of filters and compute brightness
    uint_t num_bands = aubio_filterbank_get_power(data->filterbank);
    for (uint_t j = 0; j < num_bands; ++j) {
        brightness += data->filterbank_out->data[j];
    }
    data->brightness_sum += brightness;
    data->brightness_count++;
    // std::cout << brightness << std::endl;

    // Separate into low and high beats (simple method)
    // float onset_value = aubio_onset_get_last(data->onset);
    // if (onset_value > 0) {
    //     if (brightness < 0.5) { // Example threshold for low vs high
    //         std::cout << "LOW BEAT" << std::endl;
    //         data->low_beats.push_back(onset_value);
    //     } else if(brightness > 2.0) {
    //         std::cout << "HIGH BEAT" << std::endl;
    //         data->high_beats.push_back(onset_value);
    //     }
    // }

    // Key detection (using pitch detection)
    aubio_pitch_do(data->pitch, data->in_vec, data->pitch_out);
    float key = data->pitch_out->data[0];
    if(key < 200.0 && key > 100.0){
            std::cout << "LOW BEAT" << std::endl;
    }else if(key > 1000){
            std::cout << "HIGH BEAT" << std::endl;

    }
    // std::cout << "bpm: " << data->bpm_sum/data->bpm_count << std::endl;;
    // std::cout << "Real-Time Key Frequency: " << key << " Hz" << std::endl;

    return paContinue;
}

int main() {
    // Initialize Aubio structures
    uint_t win_s = 1024; // Window size
    uint_t hop_s = 512;  // Hop size
    AudioAnalysisData analysisData;
    analysisData.tempo = new_aubio_tempo("default", win_s, hop_s, SAMPLE_RATE);
    analysisData.pitch = new_aubio_pitch("default", win_s, hop_s, SAMPLE_RATE);
    analysisData.filterbank = new_aubio_filterbank(40, win_s);
    analysisData.onset = new_aubio_onset("default", win_s, hop_s, SAMPLE_RATE);
    analysisData.pvoc = new_aubio_pvoc(win_s, hop_s); // Initialize pvoc
    analysisData.in_vec = new_fvec(hop_s);
    analysisData.fftgrain = new_cvec(win_s);          // Initialize fftgrain
    analysisData.tempo_out = new_fvec(1);
    analysisData.pitch_out = new_fvec(1);
    analysisData.filterbank_out = new_fvec(aubio_filterbank_get_power(analysisData.filterbank));
    analysisData.bpm_sum = 0.0;
    analysisData.bpm_count = 0;
    analysisData.brightness_sum = 0.0;
    analysisData.brightness_count = 0;

    // Initialize PortAudio
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    // Open default audio input stream
    PaStream *stream;
    err = Pa_OpenDefaultStream(&stream,
                               1,          // Input channels
                               0,          // Output channels
                               paFloat32,  // Sample format
                               SAMPLE_RATE,
                               FRAMES_PER_BUFFER,
                               audioCallback,
                               &analysisData);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        Pa_Terminate();
        return 1;
    }

    // Start the audio stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        Pa_Terminate();
        return 1;
    }

    // Capture audio for a period (e.g., 30 seconds)
    std::cout << "Listening and processing in real-time for 30 seconds..." << std::endl;
    Pa_Sleep(30000); // Listen for 30 seconds

    // Stop the audio stream
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        Pa_Terminate();
        return 1;
    }

    // Close the audio stream
    Pa_CloseStream(stream);
    Pa_Terminate();

    // Calculate and print average BPM and brilliance
    if (analysisData.bpm_count > 0) {
        float avg_bpm = analysisData.bpm_sum / analysisData.bpm_count;
        std::cout << "Estimated Average BPM: " << avg_bpm << std::endl;
    } else {
        std::cout << "No beats detected." << std::endl;
    }

    if (analysisData.brightness_count > 0) {
        float avg_brightness = analysisData.brightness_sum / analysisData.brightness_count;
        std::cout << "Average Brilliance (Spectral Centroid): " << avg_brightness << std::endl;
    }

    // Print low and high beat counts
    std::cout << "Low Beats Count: " << analysisData.low_beats.size() << std::endl;
    std::cout << "High Beats Count: " << analysisData.high_beats.size() << std::endl;

    // Clean up
    del_aubio_tempo(analysisData.tempo);
    del_aubio_pitch(analysisData.pitch);
    del_aubio_filterbank(analysisData.filterbank);
    del_aubio_onset(analysisData.onset);
    //del_aubio_pvoc(analysisData.pvoc);           // Clean up pvoc
    //del_fvec(analysisData.in_vec);
    //del_cvec(analysisData.fftgrain);             // Clean up fftgrain
    del_fvec(analysisData.tempo_out);
    del_fvec(analysisData.pitch_out);
    del_fvec(analysisData.filterbank_out);

    return 0;
}
