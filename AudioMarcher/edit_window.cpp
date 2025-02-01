#include "edit_window.h"

#include <portaudio/portaudio.h>

#include "global_statics.h"
#include "imgui.h"
#include "shader_loading.h"
#include "sound_processing.h"


void EditWindow::DrawFrequencyBand(FrequencyBandProcessor& band, int index)
{
    ImGui::BeginGroup();
    ImGui::PushID(index);
    ImGui::Separator();
    ImGui::Text(band.name);
    ImGui::Indent(16.f);
    ImGui::SliderFloat("Start", &band.freqStart, GlobalStatics::FREQ_START, GlobalStatics::FREQ_END);
    ImGui::SliderFloat("End", &band.freqEnd, GlobalStatics::FREQ_START, GlobalStatics::FREQ_END);
    ImGui::SliderFloat("Multiplier", &band.multiplier, 0.f, 1.f);
    ImGui::SliderFloat("Threshold", &band.threshold, 0.f, 1.f);
    ImGui::Unindent(16.f);
    ImGui::PopID();
    ImGui::EndGroup();
}

void EditWindow::DrawWindow()
{
    if (opened)
    {
        ImGui::Begin("Edit", &opened);

        if (ImGui::ListBox("Select Shader", &currentShader, GlobalStatics::SHADER_FILES.data(), static_cast<int>(GlobalStatics::SHADER_FILES.size())))
        {
            GlobalStatics::SHADER_FILE = GlobalStatics::SHADER_FILES[currentShader];
        }

        if (ImGui::Button("Load"))
        {
            LoadShader(&GlobalStatics::SHADER_PROGRAM, GlobalStatics::SHADER_LOADER, GlobalStatics::SHADER_FILE);
        }

        ImGui::SliderFloat("Smooth Time", &GlobalStatics::SMOOTH_TIME, 0.f, 1.f);
        ImGui::SliderFloat("Smooth Range", &GlobalStatics::SMOOTH_RANGE, 0.f, 1.f);

        int i = 0;
        for (auto& band : GlobalStatics::FREQ_BANDS_MAP)
        {
            DrawFrequencyBand(band.second, i);
            i++;
        }

        if (ImGui::Button("Save preset"))
        {
            SaveConfig();
        }

        DrawDevices();

        ImGui::End();
    }
}

void EditWindow::DrawDevices()
{
    if (ImGui::BeginTable("Devices", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_Sortable))
    {
        GlobalStatics::DEVICES.clear();
        int devicesCount = Pa_GetDeviceCount();
        ImGui::TableSetupColumn("Device Name", ImGuiTableColumnFlags_WidthStretch, 300);
        ImGui::TableSetupColumn("API", ImGuiTableColumnFlags_WidthStretch, 100);
        ImGui::TableSetupColumn("Channels In", ImGuiTableColumnFlags_WidthStretch, 30);
        ImGui::TableSetupColumn("Channels Out", ImGuiTableColumnFlags_WidthStretch, 30);
        ImGui::TableSetupColumn("Set", ImGuiTableColumnFlags_WidthStretch, 30);
        for (int i = 0; i < devicesCount; i++)
        {
            ImGui::TableNextColumn();
            const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
            const PaHostApiInfo* apiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
            ImGui::Text(deviceInfo->name);
            ImGui::TableNextColumn();
            ImGui::Text(apiInfo->name);
            ImGui::TableNextColumn();
            ImGui::Text("%d", deviceInfo->maxInputChannels);
            ImGui::TableNextColumn();
            ImGui::Text("%d", deviceInfo->maxOutputChannels);
            ImGui::TableNextColumn();
            ImGui::PushID(i);
            if (ImGui::Button("Set"))
            {
                GlobalStatics::DEVICE = i;
                GlobalStatics::HOST_API = deviceInfo->hostApi;
                if (GlobalStatics::STREAM != nullptr)
                {
                    Pa_StopStream(GlobalStatics::STREAM);
                    Pa_CloseStream(GlobalStatics::STREAM);
                    Pa_Terminate();
                }
                InitAudio();
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}
