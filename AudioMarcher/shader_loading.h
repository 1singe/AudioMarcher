#pragma once

#include "global_statics.h"

namespace glsl_include
{
    class ShaderLoader;
}

void FetchShaderFiles();

void LoadShader(GLuint* shaderProgram, glsl_include::ShaderLoader* shaderLoader, const char* file);

GLuint CreateShaderFromFile(const char* filename, GLenum shaderType, glsl_include::ShaderLoader*, bool useIncludes = true);

void LoadConfig(std::string file);

std::string GetShaderPathWithoutExtension(const std::string& file);

void SaveConfig();
