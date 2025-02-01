#include "input_manager.h"

#include "shader_loading.h"

void InputManager::ProcessKey(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == 82 && action == 1)
    {
        LoadShader(&GlobalStatics::SHADER_PROGRAM, GlobalStatics::SHADER_LOADER, GlobalStatics::SHADER_FILE);
    }

    if (key == 290 && action == 1)
    {
        GlobalStatics::EDIT_WINDOW->opened = !GlobalStatics::EDIT_WINDOW->opened;
    }
}
