#include "playercontroler.h"

#include "util/debug/logger.h"

PlayerControler::PlayerControler(Camera * c, const vkutil::VulkanState & state) : state(state) {
    this->camera = c;
    this->hasCursor = true;

    this->radius = 10.0;
    this->theta = asin(camera->position.z / radius);
    this->phi = atan2(camera->position.x, camera->position.y);

}

PlayerControler::~PlayerControler() {
    //dtor
}

float zAxis[3] = {0, 0, 1};

void PlayerControler::onMouseMotion(double xpos, double ypos, double dx, double dy) {

    if (!hasCursor) {

        //this->camera->rotate(-0.01 * dy, -0.01 * dx);
        //if ((theta < M_PI / 2.1 && dx >= 0) || (theta > -M_PI / 2.1 && dx <= 0))
        theta += 0.0025 * dy;
        if (theta >= M_PI / 2)
            theta = M_PI / 2;
        else if (theta <= -M_PI / 2)
            theta = - M_PI / 2;

        phi -= 0.0025 * dx;

        double ctheta = cos(theta);

        std::array<float,3> yAxisArray = {0, 1, 0};
        std::array<float,3> posArray = {(float) (radius * cos(phi) * ctheta),
                                        (float) (radius * sin(phi) * ctheta),
                                        (float) (radius * sin(theta))};

        Math::Vector<3, float> pos(posArray.data());
        Math::Vector<3, float> yAxis(yAxisArray.data());
        Math::Vector<3, float> axis = Math::cross(yAxis, -1.0f * pos);

        double s = axis.length() / pos.length();
        double c = (yAxis * (-1.0f * pos)) / pos.length();

        double alpha = atan2(c, s);

        axis = axis / axis.length();

        Math::Quaternion<float> r = Math::Quaternion<float>::fromAxisAngle(axis, alpha);

        this->camera->setPosition(pos[0], pos[1], pos[2]);
        this->camera->setRotation(r);

    }

}

void PlayerControler::onKeyboard(int key, int scancode, int action, int mods) {

    /*switch (key) {

        case GLFW_KEY_C:
            if (action != GLFW_PRESS) break;
            if (hasCursor) {
                glfwSetInputMode(state.glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            } else {
                glfwSetInputMode(state.glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            hasCursor = !hasCursor;
            break;

    }*/

}

void PlayerControler::onMouseButton(int button, int action, int mods) {

    switch (button) {

        case GLFW_MOUSE_BUTTON_LEFT:
        case GLFW_MOUSE_BUTTON_RIGHT:
        case GLFW_MOUSE_BUTTON_MIDDLE:
            if (action == GLFW_PRESS) {
                hasCursor = false;
            } else if (action == GLFW_RELEASE) {
                hasCursor = true;
            }
            break;

    }

}

void PlayerControler::onScroll(double dx, double dy) {

    logger(std::cout) << "Scroll " << dx << " " << dy << std::endl;

    this->radius -= dy * 0.05 * radius;

    if (this->radius < 0.5)
        radius = 0.5;

    else if (radius > 100)
        radius = 100;

    double ctheta = cos(theta);

        std::array<float,3> yAxisArray = {0, 1, 0};
        std::array<float,3> posArray = {(float) (radius * cos(phi) * ctheta),
                                        (float) (radius * sin(phi) * ctheta),
                                        (float) (radius * sin(theta))};

        Math::Vector<3, float> pos(posArray.data());
        Math::Vector<3, float> yAxis(yAxisArray.data());
        Math::Vector<3, float> axis = Math::cross(yAxis, -1.0f * pos);

        double s = axis.length() / pos.length();
        double c = (yAxis * (-1.0f * pos)) / pos.length();

        double alpha = atan2(c, s);

        axis = axis / axis.length();

        Math::Quaternion<float> r = Math::Quaternion<float>::fromAxisAngle(axis, alpha);

        this->camera->setPosition(pos[0], pos[1], pos[2]);
        this->camera->setRotation(r);

}
