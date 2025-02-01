#pragma once

#include <glad/glad.h>
#include <portaudio/portaudio.h>

void PrintDevice(const PaDeviceInfo* deviceInfo, int index);

void PrintHostApi(const PaHostApiInfo* hostApiInfo, int index);

void GetLogs(GLuint shader);

void GLFW_ErrorCallback(int error, const char* description);

void processAndLogAudioErrorIfAny(PaError error);
