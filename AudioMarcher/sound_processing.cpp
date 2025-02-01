#include "sound_processing.h"

#include <algorithm>
#include <complex>
#include <iostream>
#include <portaudio/pa_win_wasapi.h>

#include "global_statics.h"
#include "utils.h"

#define SILENCE 0
#define STEREO 2

#define WASAPI 3

static int audioCallback(const void* input, void* output, unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
    auto data = static_cast<AudioData*>(userData);
    CircularBuffer<float>* leftBuffer = &data->leftBuffer;
    CircularBuffer<float>* rightBuffer = &data->rightBuffer;

    auto readPointer = static_cast<const float*>(input);

    float volL = 0.f;
    float volR = 0.f;

    for (unsigned long i = 0; i < frameCount; i++)
    {
        volL = std::max(volL, std::abs(*readPointer));
        data->leftSpectrogram->input[i] = input ? *readPointer : SILENCE;
        leftBuffer->push_back(input ? *readPointer++ : SILENCE);

        if (GlobalStatics::CHANNELS_USED == STEREO)
        {
            volR = std::max(volR, std::abs(*readPointer));
            data->rightSpectrogram->input[i] = input ? *readPointer : SILENCE;
            rightBuffer->push_back(input ? *readPointer++ : SILENCE);
        }
    }
    fftw_execute(data->leftSpectrogram->p);
    fftw_execute(data->rightSpectrogram->p);

    // Processing to normalize the output
    for (int i = 0; i < GlobalStatics::BUFFER_SIZE; i++)
    {
        // 20log(intensité)/minfreq (10^-12)
        data->leftSpectrogram->processedOutput[i] = static_cast<float>(std::abs(data->leftSpectrogram->output[static_cast<int>(static_cast<float>(GlobalStatics::StartIndex()) + (static_cast<float>(i) / static_cast<float>(GlobalStatics::BUFFER_SIZE)) * static_cast<float>(GlobalStatics::SpectroSize()))]));

        data->rightSpectrogram->processedOutput[i] = static_cast<float>(std::abs(data->rightSpectrogram->output[static_cast<int>(static_cast<float>(GlobalStatics::StartIndex()) + (static_cast<float>(i) / static_cast<float>(GlobalStatics::BUFFER_SIZE)) * static_cast<float>(GlobalStatics::SpectroSize()))]));

        if (GlobalStatics::APPLY_ISOTONIC)
        {
            data->leftSpectrogram->processedOutput[i] = isotonic(data->leftSpectrogram->processedOutput[i], i, GlobalStatics::BUFFER_SIZE);
            data->rightSpectrogram->processedOutput[i] = isotonic(data->rightSpectrogram->processedOutput[i], i, GlobalStatics::BUFFER_SIZE);
        }
    }

    float workingBufferLeft[GlobalStatics::BUFFER_SIZE];
    float workingBufferRight[GlobalStatics::BUFFER_SIZE];
    std::move(data->rightSpectrogram->processedOutput, data->rightSpectrogram->processedOutput + GlobalStatics::BUFFER_SIZE, workingBufferRight);
    std::move(data->leftSpectrogram->processedOutput, data->leftSpectrogram->processedOutput + GlobalStatics::BUFFER_SIZE, workingBufferLeft);


    //Process the output
    smoothBuffer(data->rightSpectrogram->processedOutput, workingBufferRight, GlobalStatics::BUFFER_SIZE, GlobalStatics::SMOOTH_RANGE, 1.f, 10.f);
    smoothBuffer(data->leftSpectrogram->processedOutput, workingBufferLeft, GlobalStatics::BUFFER_SIZE, GlobalStatics::SMOOTH_RANGE, 1.f, 10.f);

    smoothTime(data->rightSpectrogram->lastProcessedOutput, workingBufferRight, GlobalStatics::BUFFER_SIZE, GlobalStatics::SMOOTH_TIME);
    smoothTime(data->leftSpectrogram->lastProcessedOutput, workingBufferLeft, GlobalStatics::BUFFER_SIZE, GlobalStatics::SMOOTH_TIME);

    std::move(workingBufferLeft, workingBufferLeft + GlobalStatics::BUFFER_SIZE, data->leftSpectrogram->processedOutput);
    std::move(workingBufferRight, workingBufferRight + GlobalStatics::BUFFER_SIZE, data->rightSpectrogram->processedOutput);

    std::move(workingBufferLeft, workingBufferLeft + GlobalStatics::BUFFER_SIZE, data->leftSpectrogram->lastProcessedOutput);
    std::move(workingBufferLeft, workingBufferLeft + GlobalStatics::BUFFER_SIZE, data->rightSpectrogram->lastProcessedOutput);

    data->leftVolume = volL;
    data->rightVolume = volR;
    return paContinue;
}


inline float smooth(float in, float smoothingLength, float bluriness, float smoothingFactor)
{
    return std::pow(smoothingFactor, -(1.f / bluriness) * (in / smoothingLength) * (in / smoothingLength));
}

void smoothBuffer(float* inSoundBuffer, float* outSoundBuffer, const int size, float smoothingLength, float bluriness, float smoothingFactor)
{
    smoothingLength = std::clamp(smoothingLength, 0.0f, 1.0f);
    int adjacentSamples = static_cast<int>(floor(smoothingLength * size));
    for (int i = 0; i < size; i++)
    {
        float sum = 0;
        int div = 0;
        for (int n = -adjacentSamples; n <= adjacentSamples; n++)
        {
            if (i + n < 0 || i + n > size)
            {
                continue;
            }
            div++;
            sum += (inSoundBuffer[i + n]);
        }
        outSoundBuffer[i] = sum / static_cast<float>(div);
    }
}

void smoothTime(float* lastBuffer, float* inOutBuffer, const int size, float t)
{
    for (int j = 0; j < size; j++)
    {
        inOutBuffer[j] = std::lerp(lastBuffer[j], inOutBuffer[j], t);
    }
}

float isotonic(float input, int index, float bufferSize)
{
    return std::pow(input, (static_cast<float>(index) / bufferSize) + 0.3f);
}

int remapFrequencyToIndex(float freq)
{
    return static_cast<int>(std::floor(((freq - GlobalStatics::FREQ_START) / (GlobalStatics::FREQ_END - GlobalStatics::FREQ_START) * GlobalStatics::BUFFER_SIZE)));
}

void getAverageFreqBandInline(float* buffer, FrequencyBandProcessor* band)
{
    const float freqMin = std::max(GlobalStatics::FREQ_START, band->freqStart);
    const float freqMax = std::min(GlobalStatics::FREQ_END, band->freqEnd);
    const int minI = remapFrequencyToIndex(freqMin);
    const int maxI = remapFrequencyToIndex(freqMax);
    const int nSamples = maxI - minI;
    float sum = 0.0f;
    for (int i = minI; i < maxI; i++)
    {
        sum += buffer[i];
    }

    band->averageIntensity = sum / static_cast<float>(nSamples);
}

void InitAudio()
{
    processAndLogAudioErrorIfAny(Pa_Initialize());

    std::cout << "Loaded portaudio correctly" << '\n';

    int numDevices = Pa_GetDeviceCount();

    if (numDevices < 0)
    {
        std::cout << "No audio devices found" << '\n';
        exit(EXIT_SUCCESS);
    }

    int hostApiCount = Pa_GetHostApiCount();

    if (GlobalStatics::DISPLAY_ALL_DEVICES)
    {
        for (int i = 0; i < hostApiCount; i++)
        {
            const PaHostApiInfo* apiInfo = Pa_GetHostApiInfo(i);
            PrintHostApi(apiInfo, i);
        }
    }

    if (GlobalStatics::DISPLAY_ALL_DEVICES)
    {
        for (int i = 0; i < numDevices; i++)
        {
            const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
            PrintDevice(deviceInfo, i);
        }
    }

    const int selectedDevice = GlobalStatics::DEVICE;
    const int selectedHostApi = GlobalStatics::HOST_API;
    const PaDeviceInfo* selectedDeviceInfo = Pa_GetDeviceInfo(selectedDevice);
    const PaHostApiInfo* selectedHostApiInfo = Pa_GetHostApiInfo(selectedHostApi);
    if (selectedDeviceInfo == nullptr || selectedHostApiInfo == nullptr)
    {
        std::cerr << "Could not find device: " << GlobalStatics::DEVICE << "for host api " << GlobalStatics::HOST_API << '\n';
        exit(EXIT_FAILURE);
    }


    PaStreamParameters params = {};
    params.device = selectedDevice;

    if (selectedHostApi == WASAPI)
    {
        PaWasapiStreamInfo wasapiStreamInfo = {};
        wasapiStreamInfo.size = sizeof(wasapiStreamInfo);
        wasapiStreamInfo.streamCategory = eAudioCategoryMedia;
        wasapiStreamInfo.streamOption = eStreamOptionMatchFormat;
        wasapiStreamInfo.threadPriority = eThreadPriorityAudio;
        wasapiStreamInfo.hostApiType = paWASAPI;
        params.hostApiSpecificStreamInfo = &wasapiStreamInfo;
    }
    params.channelCount = std::min(selectedDeviceInfo->maxInputChannels, GlobalStatics::CHANNELS_USED);
    params.suggestedLatency = selectedDeviceInfo->defaultLowInputLatency;
    params.sampleFormat = paFloat32;


    processAndLogAudioErrorIfAny(Pa_OpenStream(&GlobalStatics::STREAM, &params, nullptr, selectedDeviceInfo->defaultSampleRate, GlobalStatics::BUFFER_SIZE * 2, paNoFlag, audioCallback, GlobalStatics::AUDIO_DATA));

    processAndLogAudioErrorIfAny(Pa_StartStream(GlobalStatics::STREAM));
}
