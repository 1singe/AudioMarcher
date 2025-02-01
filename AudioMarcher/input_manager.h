#pragma once

struct GLFWwindow;

class InputManager
{
public:
    InputManager() = default;

    static void ProcessKey(GLFWwindow* window, int key, int scancode, int action, int mods);
};
