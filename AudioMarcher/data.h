#pragma once

#include <FFTW//fftw3.h>
#include <circular_buffer.h>

struct Spectrogram
{
    double* input;
    double* output;
    double* normalizedOutput;
    fftw_plan p;

    Spectrogram(int bufferSize)
    {
        input = new double[bufferSize];
        output = new double[bufferSize];
        normalizedOutput = new double[bufferSize];
        p = fftw_plan_r2r_1d(bufferSize, input, output, FFTW_R2HC, FFTW_ESTIMATE);
    }

    ~Spectrogram()
    {
        delete(input);
        delete(output);
        delete(normalizedOutput);
        fftw_destroy_plan(p);
    }
};

struct AudioData
{
    CircularBuffer<float> leftBuffer;
    CircularBuffer<float> rightBuffer;
    float leftVolume;
    float rightVolume;
    Spectrogram* leftSpectrogram = nullptr;
    Spectrogram* rightSpectrogram = nullptr;

    AudioData(int bufferSize):
        leftBuffer(CircularBuffer<float>(bufferSize)),
        rightBuffer(CircularBuffer<float>(bufferSize)),
        leftVolume(0.f),
        rightVolume(0.f),
        leftSpectrogram(new Spectrogram(bufferSize)),
        rightSpectrogram(new Spectrogram(bufferSize))
    {
    }

    ~AudioData()
    {
        delete leftSpectrogram;
        delete rightSpectrogram;
    }
};
