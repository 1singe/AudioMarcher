#pragma once

#include <glad/glad.h>
#include <portaudio/portaudio.h>

void PrintDevice(const PaDeviceInfo* deviceInfo, int index);

void PrintHostApi(const PaHostApiInfo* hostApiInfo, int index);

void GetLogs(GLuint shader);