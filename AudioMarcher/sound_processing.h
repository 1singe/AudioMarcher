#pragma once
#include <portaudio/portaudio.h>

struct FrequencyBandProcessor
{
    const char* name;
    int channel;
    float freqStart;
    float freqEnd;
    float multiplier;
    float threshold;
    float averageIntensity;
};

static int audioCallback(const void* input, void* output, unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData);

// smoothing factor of 1 is no processing ;)
inline float smooth(float in, float smoothingLength, float bluriness, float smoothingFactor);

void smoothBuffer(float* inSoundBuffer, float* outSoundBuffer, int size, float smoothingLengthNormalized, float bluriness, float smoothingFactor);

void smoothTime(float* lastBuffer, float* inOutBuffer, int size, float t);

float isotonic(float input, int index, float bufferSize);

void getAverageFreqBandInline(float* buffer, FrequencyBandProcessor* band);

void InitAudio();
