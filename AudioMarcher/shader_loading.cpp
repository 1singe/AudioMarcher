#include "shader_loading.h"

#include <filesystem>
#include <fstream>
#include <string>

#include "global_statics.h"
#include "glsl_include.hpp"
#include "utils.h"

GLuint CreateShaderFromFile(const char* filename, GLenum shaderType, glsl_include::ShaderLoader* shaderLoader, bool useIncludes)
{
    std::string pathString = "shaders/";
    std::string fileString = filename;
    std::string fullPathString = pathString + fileString;
    const char* fullPathCharray = fullPathString.c_str();

    if (useIncludes)
    {
        std::string stringBuffer = (shaderLoader->load_shader(fullPathString));
        auto buffer = stringBuffer.data();
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


void FetchShaderFiles()
{
    std::string path = "C:/Users/foura/Documents/Visualizer/AudioMarcher/shaders";
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(path))
    {
        if (entry.is_regular_file())
        {
            std::string filenameStr = entry.path().filename().string();
            int size = filenameStr.size();
            auto filename = new char[size + 1];
            memcpy(filename, &filenameStr[0], size + 1);
            GlobalStatics::SHADER_FILES.push_back(filename);
        }
    }
}

void LoadShader(GLuint* shaderProgram, glsl_include::ShaderLoader* shaderLoader, const char* file)
{
    glDeleteProgram(*shaderProgram);
    *shaderProgram = glCreateProgram();
    GLuint frag = CreateShaderFromFile(file, GL_FRAGMENT_SHADER, shaderLoader);
    GLuint vert = CreateShaderFromFile("vertex_shader.glsl", GL_VERTEX_SHADER, shaderLoader);
    glCompileShader(frag);
    GetLogs(frag);
    glCompileShader(vert);
    GetLogs(vert);
    LoadConfig(file);
    glAttachShader(*shaderProgram, frag);
    glAttachShader(*shaderProgram, vert);
    glDeleteShader(frag);
    glDeleteShader(vert);
    glLinkProgram(*shaderProgram);
    glUseProgram(*shaderProgram);
    glDetachShader(*shaderProgram, frag);
    glDetachShader(*shaderProgram, vert);
}

std::string GetShaderPathWithoutExtension(const std::string& file)
{
    std::istringstream filename(file);
    std::string fileWithoutExtension;
    std::getline(filename, fileWithoutExtension, '.');
    std::string completePath = "shaders/" + fileWithoutExtension;

    return completePath;
}

void LoadConfig(std::string file)
{
    std::string completePath = GetShaderPathWithoutExtension(file) + ".ini";
    GlobalStatics::CONFIG_READER = new INIReader(completePath);

    if (GlobalStatics::CONFIG_READER->ParseError() != 0)
    {
        std::cerr << "Can't load " << completePath << '\n';
    }

    for (std::pair<const std::string, FrequencyBandProcessor>& entry : GlobalStatics::FREQ_BANDS_MAP)
    {
        entry.second.multiplier = GlobalStatics::CONFIG_READER->GetFloat(entry.first, "multiplier", 1.f);
        entry.second.threshold = GlobalStatics::CONFIG_READER->GetFloat(entry.first, "threshold", 1.f);
        entry.second.freqStart = GlobalStatics::CONFIG_READER->GetFloat(entry.first, "freqStart", 20.f);
        entry.second.freqEnd = GlobalStatics::CONFIG_READER->GetFloat(entry.first, "freqEnd", 20000.f);
    }
}

void SaveConfig()
{
    const auto fileStr = std::string(GlobalStatics::SHADER_FILE);
    std::string configFile = GetShaderPathWithoutExtension(fileStr) + ".ini";
    std::ofstream outFile(configFile);
    for (const std::pair<const std::string, FrequencyBandProcessor>& entry : GlobalStatics::FREQ_BANDS_MAP)
    {
        outFile << '[' << entry.first << "]" << '\n';
        outFile << "multiplier=" << entry.second.multiplier << '\n';
        outFile << "threshold=" << entry.second.threshold << '\n';
        outFile << "freqStart=" << entry.second.freqStart << '\n';
        outFile << "freqEnd=" << entry.second.freqEnd << '\n';
        outFile << '\n';

    }

    outFile.close();
}
