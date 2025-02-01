#pragma once

#include <algorithm>
#include <string>
#include <cmath>
#include <vector>
#include <glad/glad.h>
#include <portaudio/portaudio.h>

#include "sound_processing.h"
#include "edit_window.h"

#include "data.h"
#include "INIReader.h"

struct AudioData;

namespace glsl_include
{
    class ShaderLoader;
}

namespace GlobalStatics
{
    constexpr int BUFFER_SIZE = 256;

    inline float SAMPLE_RATE = 48000.0f;
    inline int CHANNELS_USED = 2;

    inline float FREQ_START = 20.f;
    inline float FREQ_END = 20000.f;

    inline int HOST_API = 2;
    inline int DEVICE = 13;

    inline int WIDTH = 1600;
    inline int HEIGHT = 900;

    inline float SMOOTH_TIME = 0.5f;
    inline float SMOOTH_RANGE = 0.00f;
    inline bool APPLY_ISOTONIC = true;

    inline std::string INCLUDE_KEYWORD = "#include";
    inline const char* SHADER_FILE = "fractal_hypno.glsl";

    inline GLuint SHADER_PROGRAM;

    inline glsl_include::ShaderLoader* SHADER_LOADER = nullptr;
    inline INIReader* CONFIG_READER = nullptr;

    inline AudioData* AUDIO_DATA = new AudioData(BUFFER_SIZE);

    inline EditWindow* EDIT_WINDOW = nullptr;

    inline std::vector<const char*> SHADER_FILES = std::vector<const char*>();
    inline std::vector<const char*> DEVICES = std::vector<const char*>();

    inline PaStream* STREAM = nullptr;


    inline std::map<std::string, FrequencyBandProcessor> FREQ_BANDS_MAP = {
        {"subL", {"subL", 0, 20, 200, 1.f, 1.f, 1.f}},
        {"subR", {"subR", 1, 20, 200, 1.f, 1.f, 1.f}},
        {"lowL", {"lowL", 0, 200, 800, 1.f, 1.f, 1.f}},
        {"lowR", {"lowR", 1, 200, 800, 1.f, 1.f, 1.f}},
        {"midL", {"midL", 0, 800, 3000, 1.f, 1.f, 1.f}},
        {"midR", {"midR", 1, 800, 3000, 1.f, 1.f, 1.f}},
        {"highL", {"highL", 0, 3000, 12000, 1.f, 1.f, 1.f}},
        {"highR", {"highR", 1, 3000, 12000, 1.f, 1.f, 1.f}},
    };

    //inline FrequencyBandProcessor FREQ_BANDS[] = {
    //    {"subL", 0, 20, 200, 1.f, 1.f, 1.f},
    //    {"subR", 1, 20, 200, 1.f, 1.f, 1.f},
    //
    //    {"lowL", 0, 200, 800, 1.f, 1.f, 1.f},
    //    {"lowR", 1, 200, 800, 1.f, 1.f, 1.f},
    //
    //    {"midL", 0, 800, 3000, 1.f, 1.f, 1.f},
    //    {"midR", 1, 800, 3000, 1.f, 1.f, 1.f},
    //
    //    {"highL", 0, 3000, 12000, 1.f, 1.f, 1.f},
    //    {"highR", 1, 3000, 12000, 1.f, 1.f, 1.f}
    //};


    inline float SampleRatio()
    {
        return static_cast<float>(BUFFER_SIZE) / SAMPLE_RATE;
    }

    inline int StartIndex()
    {
        return static_cast<int>(std::ceil(FREQ_START * SampleRatio()));
    }

    inline int SpectroSize()
    {
        return std::min(static_cast<int>(std::ceil(SampleRatio() * FREQ_END)), BUFFER_SIZE) - StartIndex();
    }

    // DEBUG

    inline bool DISPLAY_ALL_DEVICES = false;
    inline bool DISPLAY_ALL_HOSTAPI = false;
}
