#pragma once

struct FrequencyBandProcessor;

struct EditWindow
{
    bool opened = false;
    bool audio = false;
    int selectedShader = 0;
    const char* shaderFiles = nullptr;
    int currentShader = 0;

    void DrawFrequencyBand(FrequencyBandProcessor& band, int index);
    void DrawWindow();
    void DrawDevices();
};
