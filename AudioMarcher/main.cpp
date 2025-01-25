#include <fstream>
#include "utils.h"
#include <filesystem>
#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<portaudio/portaudio.h>
#include "data.h"
#include<FFTW/fftw3.h>
#include "glsl_include.hpp"
#include "circular_buffer.h"
#include <portaudio//pa_win_wasapi.h>

#define SAMPLE_RATE 48000.0
#define BUFFER_SIZE 512

#define STEREO 2
#define MONO 1
#define NONE 0

#define LEFT 0
#define RIGHT 1

#define NUM_CHANNELS STEREO

#define SILENCE 0

#define USE_DEFAULT_DEVICES 0

#define DISPLAY_ALL_DEVICE 1
#define DISPLAY_ALL_HOSTAPI 1

#define WASAPI 3

#define FREQ_START 20
#define FREQ_END 20000


int m_width, m_height;
int m_channels[NUM_CHANNELS] = {LEFT, RIGHT};
std::string m_includeKeyword = "#include";
constexpr int mc_hostApi = 2;
constexpr int mc_device = 13;

constexpr double mc_sampleRatio = BUFFER_SIZE / SAMPLE_RATE;
const int m_startIndex = std::ceil(FREQ_START * mc_sampleRatio);
const int m_spectroSize = std::min(static_cast<int>(std::ceil(mc_sampleRatio * FREQ_END)), BUFFER_SIZE) - m_startIndex;


glsl_include::ShaderLoader* m_shaderLoader = nullptr;

void window_size_callback(GLFWwindow* window, int width, int height)
{
    m_width = width;
    m_height = height;
    glViewport(0, 0, width, height);
}

GLuint CreateShaderFromFile(const char* filename, GLenum shaderType, bool useIncludes = true)
{
    std::string pathString = "shaders/";
    std::string fileString = filename;
    std::string fullPathString = pathString + fileString;
    const char* fullPathCharray = fullPathString.c_str();

    if (useIncludes)
    {
        std::string stringBuffer = (m_shaderLoader->load_shader(fullPathString));
        auto buffer = (char*)stringBuffer.data();
        GLuint shader = glCreateShader(shaderType);
        glShaderSource(shader, 1, &buffer, nullptr);
        return shader;
    }
    std::ifstream infile;
    infile.open(fullPathCharray, std::ios_base::in | std::ios_base::binary);
    if (infile)
    {
        infile.seekg(0, std::ifstream::end);
        int len = infile.tellg();
        infile.seekg(0, std::ifstream::beg);
        auto buffer = new char[len + 1];
        infile.read(buffer, len);
        buffer[len] = '\0';
        GLuint shader = glCreateShader(shaderType);
        glShaderSource(shader, 1, &buffer, nullptr);
        return shader;
    }
    return -1;
}


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

        if (NUM_CHANNELS == STEREO)
        {
            volR = std::max(volR, std::abs(*readPointer));
            data->rightSpectrogram->input[i] = input ? *readPointer : SILENCE;
            rightBuffer->push_back(input ? *readPointer++ : SILENCE);
        }
    }
    fftw_execute(data->leftSpectrogram->p);
    fftw_execute(data->rightSpectrogram->p);

    // Processing to normalize the output
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        data->leftSpectrogram->normalizedOutput[i] = data->leftSpectrogram->output[static_cast<int>(m_startIndex + std::pow(i / static_cast<double>(BUFFER_SIZE), 3) * m_spectroSize)];
        data->rightSpectrogram->normalizedOutput[i] = data->leftSpectrogram->output[static_cast<int>(m_startIndex + std::pow(i / static_cast<double>(BUFFER_SIZE), 3) * m_spectroSize)];
    }

    data->leftVolume = volL;
    data->rightVolume = volR;
    return paContinue;
}

void processAndLogAudioErrorIfAny(PaError error)
{
    if (error != paNoError)
    {
        std::cerr << "PortAudio error:\n" << Pa_GetErrorText(error) << '\n';
    }
}

int main()
{
    m_shaderLoader = new glsl_include::ShaderLoader(m_includeKeyword.c_str());

    int selectedDevice = -1;
    const PaDeviceInfo* selectedDeviceInfo = nullptr;
    int selectedHostApi = -1;
    const PaHostApiInfo* selectedHostApiInfo = nullptr;

    processAndLogAudioErrorIfAny(Pa_Initialize());

    std::cout << "Loaded portaudio correctly" << '\n';

    int numDevices = Pa_GetDeviceCount();

    if (numDevices < 0)
    {
        std::cout << "No audio devices found" << '\n';
        exit(EXIT_SUCCESS);
    }

    int hostApiCount = Pa_GetHostApiCount();

#if DISPLAY_ALL_HOSTAPI
    for (int i = 0; i < hostApiCount; i++)
    {
        const PaHostApiInfo* apiInfo = Pa_GetHostApiInfo(i);
        PrintHostApi(apiInfo, i);
    }
#endif

#if DISPLAY_ALL_DEVICE
    for (int i = 0; i < numDevices; i++)
    {
        const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
        PrintDevice(deviceInfo, i);
    }
#endif

    // Query available audio devices


    if (USE_DEFAULT_DEVICES)
    {
        selectedDevice = Pa_GetDefaultInputDevice();
        selectedDeviceInfo = Pa_GetDeviceInfo(selectedDevice);
    }
    selectedDevice = mc_device;
    selectedHostApi = mc_hostApi;
    selectedDeviceInfo = Pa_GetDeviceInfo(selectedDevice);
    selectedHostApiInfo = Pa_GetHostApiInfo(selectedHostApi);
    if (selectedDeviceInfo == nullptr || selectedHostApiInfo == nullptr)
    {
        std::cerr << "Could not find device: " << mc_device << "for host api " << mc_hostApi << '\n';
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
    params.channelCount = std::min(selectedDeviceInfo->maxInputChannels, NUM_CHANNELS);
    params.suggestedLatency = selectedDeviceInfo->defaultLowInputLatency;
    params.sampleFormat = paFloat32;

    auto audioData = AudioData(BUFFER_SIZE);


    PaStream* stream = nullptr;
    processAndLogAudioErrorIfAny(Pa_OpenStream(&stream, &params, nullptr, selectedDeviceInfo->defaultSampleRate, BUFFER_SIZE * 2, paNoFlag, audioCallback, &audioData));

    processAndLogAudioErrorIfAny(Pa_StartStream(stream));

    if (!glfwInit())
    {
        std::cerr << "Failed to init GLFW" << '\n';
        processAndLogAudioErrorIfAny(Pa_Terminate());
        exit(EXIT_FAILURE);
    }

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    m_width = mode->width / 2;
    m_height = mode->height / 2;

    GLFWwindow* window = glfwCreateWindow(m_width, m_height, u8"AudioMarcher", nullptr, nullptr);

    if (window == nullptr)
    {
        std::cerr << "Failed to create window" << '\n';
        glfwTerminate();
        processAndLogAudioErrorIfAny(Pa_Terminate());
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, true);

    if (!gladLoadGL())
    {
        std::cerr << "Failed to load glad" << '\n';
        glfwDestroyWindow(window);
        glfwTerminate();
        processAndLogAudioErrorIfAny(Pa_Terminate());
        exit(EXIT_FAILURE);
    }

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);


    GLuint shaderProgram = glCreateProgram();

    GLuint fragmentShader = CreateShaderFromFile("fragment_shader.glsl", GL_FRAGMENT_SHADER);
    glCompileShader(fragmentShader);
    GetLogs(fragmentShader);
    glAttachShader(shaderProgram, fragmentShader);
    glDeleteShader(fragmentShader);

    GLuint vertexShader = CreateShaderFromFile("vertex_shader.glsl", GL_VERTEX_SHADER);
    glCompileShader(vertexShader);
    GetLogs(vertexShader);
    glAttachShader(shaderProgram, vertexShader);
    glDeleteShader(vertexShader);

    glLinkProgram(shaderProgram);
    glDetachShader(shaderProgram, fragmentShader);
    glDetachShader(shaderProgram, vertexShader);

    //GLuint textures[2];
    //glGenTextures(2, textures);

    glfwSetWindowSizeCallback(window, window_size_callback);

    do
    {
        glfwPollEvents();

        glClearColor(.0f, .08f, 0.13f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // parameters

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);


        GLint leftSoundBufferLocation = glGetUniformLocation(shaderProgram, "sound_buffer_left");
        GLint rightSoundBufferLocation = glGetUniformLocation(shaderProgram, "sound_buffer_right");
        float left[BUFFER_SIZE];
        float right[BUFFER_SIZE];
        std::move(audioData.leftBuffer.data(), audioData.leftBuffer.data() + BUFFER_SIZE, left);
        std::move(audioData.rightBuffer.data(), audioData.rightBuffer.data() + BUFFER_SIZE, right);


        GLint leftSoundSpectroLocation = glGetUniformLocation(shaderProgram, "sound_spectro_left");
        GLint righttSoundSpectroLocation = glGetUniformLocation(shaderProgram, "sound_spectro_right");

        float leftSpectro[BUFFER_SIZE];
        float rightSpectro[BUFFER_SIZE];

        std::move(audioData.leftSpectrogram->normalizedOutput, audioData.leftSpectrogram->normalizedOutput + BUFFER_SIZE, leftSpectro);
        std::move(audioData.rightSpectrogram->normalizedOutput, audioData.rightSpectrogram->normalizedOutput + BUFFER_SIZE, rightSpectro);

        //glActiveTexture(GL_TEXTURE0);
        //glBindTexture(GL_TEXTURE_1D, textures[0]);
        //glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        //glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        //glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        //glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        //glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F_ARB, BUFFER_SIZE, 0, GL_RGBA32F_ARB, GL_FLOAT, leftSpectro);
        //glBindTexture(GL_TEXTURE_1D, 0);
        //
        //glActiveTexture(GL_TEXTURE1);
        //glBindTexture(GL_TEXTURE_1D, textures[1]);
        //glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        //glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        //glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        //glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        //glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, BUFFER_SIZE, 0, GL_RED, GL_FLOAT, rightSpectro);
        //glBindTexture(GL_TEXTURE_1D, 1);
        //glUniform1i(glGetUniformLocation(shaderProgram, "rightFreq"), 3);
        //glUniform1i(glGetUniformLocation(shaderProgram, "leftFreq"), 4);

        glUniform1fv(leftSoundSpectroLocation, BUFFER_SIZE, leftSpectro);
        glUniform1fv(righttSoundSpectroLocation, BUFFER_SIZE, rightSpectro);

        glUniform1fv(leftSoundBufferLocation, BUFFER_SIZE, left);
        glUniform1fv(rightSoundBufferLocation, BUFFER_SIZE, right);

        GLint timeLocation = glGetUniformLocation(shaderProgram, "time");
        glUniform1f(timeLocation, static_cast<float>(glfwGetTime()));

        GLint volumeLocation = glGetUniformLocation(shaderProgram, "volume");
        glUniform2f(volumeLocation, audioData.leftVolume, audioData.rightVolume);

        GLint resolutionLocation = glGetUniformLocation(shaderProgram, "resolution");
        glUniform2f(resolutionLocation, static_cast<GLfloat>(m_width), static_cast<GLfloat>(m_height));

        glfwSwapBuffers(window);
    }
    while (!glfwWindowShouldClose(window) && !glfwGetKey(window, GLFW_KEY_ESCAPE));

    //glDeleteTextures(2, textures);
    glBindVertexArray(0);
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    delete(m_shaderLoader);
    delete audioData.leftSpectrogram;
    delete audioData.rightSpectrogram;
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    processAndLogAudioErrorIfAny(Pa_Terminate());
    exit(EXIT_SUCCESS);
}
