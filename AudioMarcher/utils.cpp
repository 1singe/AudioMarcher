#include "utils.h"

#include <iostream>
#include <string>

void PrintDevice(const PaDeviceInfo* deviceInfo, int index)
{
    const std::string stringName = deviceInfo->name;
    std::cout << "Device " << index << (index == Pa_GetDefaultInputDevice() ? " (Default Input)" : "") << (index == Pa_GetDefaultOutputDevice() ? " (Default Output)" : "") << ":\n";
    std::cout << '\t' << "name: " << stringName << '\n';
    std::cout << '\t' << "in: " << deviceInfo->maxInputChannels << '\n';
    std::cout << '\t' << "out: " << deviceInfo->maxOutputChannels << '\n';
    std::cout << '\t' << "host api: " << deviceInfo->hostApi << '\n';
    std::cout << '\t' << "default sample rate: " << deviceInfo->defaultSampleRate << '\n';
    std::cout << '\t' << "default low input latency: " << deviceInfo->defaultLowInputLatency << '\n';
    std::cout << '\t' << "default high input latency: " << deviceInfo->defaultHighInputLatency << '\n';
    std::cout << '\t' << "default low output latency: " << deviceInfo->defaultLowOutputLatency << '\n';
    std::cout << '\t' << "default high output latency: " << deviceInfo->defaultHighOutputLatency << '\n';
}

void PrintHostApi(const PaHostApiInfo* hostApiInfo, int index)
{
    std::cout << "Host Api " << index << (Pa_GetDefaultHostApi() == index ? " (Default)" : "") << ":\n";
    std::cout << '\t' << "name: " << hostApiInfo->name << '\n';
    std::cout << '\t' << "type: " << hostApiInfo->type << '\n';
    std::cout << '\t' << "default in: " << Pa_GetDeviceInfo(hostApiInfo->defaultInputDevice)->name << '\n';
    std::cout << '\t' << "default out: " << Pa_GetDeviceInfo(hostApiInfo->defaultOutputDevice)->name << '\n';
}

void GetLogs(GLuint shader)
{
    GLint result;
    GLint logLength;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

    if (result)
    {
        std::cout << "Compilation success." << '\n';
    }
    else
    {
        std::cout << "Compilation failed." << '\n';
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        auto info = static_cast<char*>(malloc(logLength));
        glGetShaderInfoLog(shader, logLength, &logLength, info);
        std::cerr << info << '\n';
    }
}