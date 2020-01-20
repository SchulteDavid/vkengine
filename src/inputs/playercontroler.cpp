#include "playercontroler.h"

PlayerControler::PlayerControler(Camera * c, const vkutil::VulkanState & state) : state(state) {
    this->camera = c;
    this->hasCursor = true;
}

PlayerControler::~PlayerControler() {
    //dtor
}

float zAxis[3] = {0, 0, 1};

void PlayerControler::onMouseMotion(double xpos, double ypos, double dx, double dy) {

    if (!hasCursor)
        this->camera->rotate(-0.01 * dy, -0.01 * dx);

}

void PlayerControler::onKeyboard(int key, int scancode, int action, int mods) {

    switch (key) {

        case GLFW_KEY_C:
            if (action != GLFW_PRESS) break;
            if (hasCursor) {
                glfwSetInputMode(state.glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            } else {
                glfwSetInputMode(state.glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            hasCursor = !hasCursor;
            break;

    }

}
