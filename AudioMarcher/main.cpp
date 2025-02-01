#include <fstream>
#include "utils.h"
#include <filesystem>
#include<iostream>
#include<GLFW/glfw3.h>
#include<portaudio/portaudio.h>
#include "data.h"
#include "glsl_include.hpp"
#include "circular_buffer.h"
#include <portaudio//pa_win_wasapi.h>

#include "edit_window.h"
#include "sound_processing.h"
#include "input_manager.h"

#include "imgui.h"
#include "shader_loading.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "global_statics.h"
#include "imgui_internal.h"


void window_size_callback(GLFWwindow* window, int width, int height)
{
    if (!width || !height)
        return;
    GlobalStatics::WIDTH = width;
    GlobalStatics::HEIGHT = height;
    glViewport(0, 0, width, height);
}

int main()
{
    GlobalStatics::SHADER_LOADER = new glsl_include::ShaderLoader(GlobalStatics::INCLUDE_KEYWORD.c_str());
    GlobalStatics::CONFIG_READER = new INIReader();

    GlobalStatics::EDIT_WINDOW = new EditWindow();

    int selectedDevice = -1;
    const PaDeviceInfo* selectedDeviceInfo = nullptr;
    int selectedHostApi = -1;
    const PaHostApiInfo* selectedHostApiInfo = nullptr;

    glfwSetErrorCallback(GLFW_ErrorCallback);

    InitAudio();

    if (!glfwInit())
    {
        std::cerr << "Failed to init GLFW" << '\n';
        processAndLogAudioErrorIfAny(Pa_Terminate());
        exit(EXIT_FAILURE);
    }


    GLFWwindow* window = glfwCreateWindow(GlobalStatics::WIDTH, GlobalStatics::HEIGHT, "AudioMarcher", nullptr, nullptr);

    if (window == nullptr)
    {
        std::cerr << "Failed to create window" << '\n';
        glfwTerminate();
        processAndLogAudioErrorIfAny(Pa_Terminate());
        exit(EXIT_FAILURE);
    }


    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

    glfwMakeContextCurrent(window);

    FetchShaderFiles();

    IMGUI_CHECKVERSION();
    ImGuiContext* context = ImGui::CreateContext();
    context->CurrentDpiScale = 2.f;

    ImGui::StyleColorsDark();

    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, true);

    glfwSetKeyCallback(window, InputManager::ProcessKey);

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


    GlobalStatics::SHADER_PROGRAM = glCreateProgram();

    GLuint fragmentShader = CreateShaderFromFile(GlobalStatics::SHADER_FILE, GL_FRAGMENT_SHADER, GlobalStatics::SHADER_LOADER);
    glCompileShader(fragmentShader);
    GetLogs(fragmentShader);
    glAttachShader(GlobalStatics::SHADER_PROGRAM, fragmentShader);
    glDeleteShader(fragmentShader);

    GLuint vertexShader = CreateShaderFromFile("vertex_shader.glsl", GL_VERTEX_SHADER, GlobalStatics::SHADER_LOADER);
    glCompileShader(vertexShader);
    GetLogs(vertexShader);
    glAttachShader(GlobalStatics::SHADER_PROGRAM, vertexShader);
    glDeleteShader(vertexShader);

    glLinkProgram(GlobalStatics::SHADER_PROGRAM);
    glDetachShader(GlobalStatics::SHADER_PROGRAM, fragmentShader);
    glDetachShader(GlobalStatics::SHADER_PROGRAM, vertexShader);

    glfwSetWindowSizeCallback(window, window_size_callback);

    ImGuiIO& io = ImGui::GetIO();
    ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(1.2f);
    style.WindowRounding = 16.f;
    ImGui::GetIO().FontGlobalScale = 1.2f;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");

    while (!glfwWindowShouldClose(window) && !glfwGetKey(window, GLFW_KEY_ESCAPE))
    {
        glfwPollEvents();

        glClearColor(0.02f, 0.05f, 0.16f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glUseProgram(GlobalStatics::SHADER_PROGRAM);

        // parameters

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);


        GLint leftSoundBufferLocation = glGetUniformLocation(GlobalStatics::SHADER_PROGRAM, "sound_buffer_left");
        GLint rightSoundBufferLocation = glGetUniformLocation(GlobalStatics::SHADER_PROGRAM, "sound_buffer_right");
        float left[GlobalStatics::BUFFER_SIZE];
        float right[GlobalStatics::BUFFER_SIZE];
        std::move(GlobalStatics::AUDIO_DATA->leftBuffer.data(), GlobalStatics::AUDIO_DATA->leftBuffer.data() + GlobalStatics::BUFFER_SIZE, left);
        std::move(GlobalStatics::AUDIO_DATA->rightBuffer.data(), GlobalStatics::AUDIO_DATA->rightBuffer.data() + GlobalStatics::BUFFER_SIZE, right);

        GLint leftSoundSpectroLocation = glGetUniformLocation(GlobalStatics::SHADER_PROGRAM, "sound_spectro_left");
        GLint righttSoundSpectroLocation = glGetUniformLocation(GlobalStatics::SHADER_PROGRAM, "sound_spectro_right");

        float leftSpectro[GlobalStatics::BUFFER_SIZE];
        float rightSpectro[GlobalStatics::BUFFER_SIZE];

        std::move(GlobalStatics::AUDIO_DATA->leftSpectrogram->processedOutput, GlobalStatics::AUDIO_DATA->leftSpectrogram->processedOutput + GlobalStatics::BUFFER_SIZE, leftSpectro);
        std::move(GlobalStatics::AUDIO_DATA->rightSpectrogram->processedOutput, GlobalStatics::AUDIO_DATA->rightSpectrogram->processedOutput + GlobalStatics::BUFFER_SIZE, rightSpectro);

        glUniform1fv(leftSoundSpectroLocation, GlobalStatics::BUFFER_SIZE, reinterpret_cast<float*>(leftSpectro));
        glUniform1fv(righttSoundSpectroLocation, GlobalStatics::BUFFER_SIZE, reinterpret_cast<float*>(rightSpectro));

        glUniform1fv(leftSoundBufferLocation, GlobalStatics::BUFFER_SIZE, reinterpret_cast<float*>(left));
        glUniform1fv(rightSoundBufferLocation, GlobalStatics::BUFFER_SIZE, reinterpret_cast<float*>(right));

        GLint timeLocation = glGetUniformLocation(GlobalStatics::SHADER_PROGRAM, "time");
        glUniform1f(timeLocation, static_cast<float>(glfwGetTime()));

        GLint volumeLocation = glGetUniformLocation(GlobalStatics::SHADER_PROGRAM, "volume");
        glUniform2f(volumeLocation, GlobalStatics::AUDIO_DATA->leftVolume, GlobalStatics::AUDIO_DATA->rightVolume);

        GLint resolutionLocation = glGetUniformLocation(GlobalStatics::SHADER_PROGRAM, "resolution");
        glUniform2f(resolutionLocation, static_cast<GLfloat>(GlobalStatics::WIDTH), static_cast<GLfloat>(GlobalStatics::HEIGHT));

        // Freq bands
        for (auto& band : GlobalStatics::FREQ_BANDS_MAP)
        {
            getAverageFreqBandInline(band.second.channel ? GlobalStatics::AUDIO_DATA->rightSpectrogram->processedOutput : GlobalStatics::AUDIO_DATA->leftSpectrogram->processedOutput, &band.second);
            GLint freqLocation = glGetUniformLocation(GlobalStatics::SHADER_PROGRAM, band.second.name);
            glUniform3f(freqLocation, band.second.averageIntensity, band.second.multiplier, band.second.threshold);
        }

        GlobalStatics::EDIT_WINDOW->DrawWindow();

        ImGui::EndFrame();
        ImGui::Render();
        glViewport(0, 0, GlobalStatics::WIDTH, GlobalStatics::HEIGHT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }

    glBindVertexArray(0);
    glDeleteProgram(GlobalStatics::SHADER_PROGRAM);
    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    delete(GlobalStatics::SHADER_LOADER);
    Pa_StopStream(GlobalStatics::STREAM);
    Pa_CloseStream(GlobalStatics::STREAM);
    processAndLogAudioErrorIfAny(Pa_Terminate());

    exit(EXIT_SUCCESS);
}
