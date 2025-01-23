#include <fstream>
#include <filesystem>
#include<iostream>
#include <vector>
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include "glsl_include.hpp"

int m_width, m_height;
glsl_include::ShaderLoader* m_shaderLoader = nullptr;

void window_size_callback(GLFWwindow* window, int width, int height)
{
    m_width = width;
    m_height = height;
    glViewport(0, 0, width, height);
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

int main()
{
    m_shaderLoader = new glsl_include::ShaderLoader("#include");


    if (!glfwInit())
    {
        std::cerr << "Failed to init GLFW" << '\n';
        return 1;
    }

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    m_width = mode->width / 2;
    m_height = mode->height / 2;

    GLFWwindow* window = glfwCreateWindow(m_width, m_height, u8"AudioMarcher", nullptr, nullptr);

    if (window == nullptr)
    {
        std::cerr << "Failed to create window" << '\n';
        glfwTerminate();
        return 1;
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
        return 1;
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

    glfwSetWindowSizeCallback(window, window_size_callback);

    do
    {
        glfwPollEvents();

        glClearColor(.0f, .0f, 1.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // parameters

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);

        GLint timeLocation = glGetUniformLocation(shaderProgram, "time");
        glUniform1f(timeLocation, static_cast<float>(glfwGetTime()));
        GLint resolutionLocation = glGetUniformLocation(shaderProgram, "resolution");
        glUniform2f(resolutionLocation, static_cast<GLfloat>(m_width), static_cast<GLfloat>(m_height));

        glfwSwapBuffers(window);
    }
    while (!glfwWindowShouldClose(window) && !glfwGetKey(window, GLFW_KEY_ESCAPE));

    glBindVertexArray(0);
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    delete(m_shaderLoader);
    exit(EXIT_SUCCESS);
}
