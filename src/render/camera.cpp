#include "camera.h"

#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace Math;

Camera::Camera(float fov, float near, float far, float aspect, glm::vec3 pos, Math::Quaternion<float> rot) {

    this->updateProjection(fov, near, far, aspect);

    this->position = pos;
    this->rotation = rot;

    pitch = 0;
    yaw = 0;

    this->updateView();

}

Camera::~Camera()
{
    //dtor
}

glm::mat4 Camera::getProjection() {
    return this->projection;
}

glm::mat4 Camera::getView() {
    return this->view;
}

void Camera::move(float dx, float dy, float dz) {

    position.x += dx;
    position.y += dy;
    position.z += dz;

    updateView();

}

void Camera::move(glm::vec3 d) {
    position += d;
    updateView();
}

void Camera::setPosition(float x, float y, float z) {

    position.x = x;
    position.y = y;
    position.z = z;

    updateView();

}

void Camera::setRotation(Math::Quaternion<float> r) {
    rotation = r;
    updateView();
}

void Camera::rotate(Math::Quaternion<float> d) {

    rotation = d * rotation;
    updateView();

}

void Camera::rotate(float dp, float dy) {

    std::array<float, 3> vAxis = {1, 0, 0};
    std::array<float, 3> hAxis = {0, 0, 1};

    this->pitch += dp;

    if (pitch <= -M_PI /2) {
        pitch = -M_PI / 2;
    } else if (pitch >= M_PI /2) {
        pitch = M_PI / 2;
    }

    this->yaw += dy;

    Quaternion<float> v(Vector<3, float>(vAxis.data()), pitch);
    Quaternion<float> h(Vector<3, float>(hAxis.data()), yaw);

    this->rotation = v * h;

    updateView();

}

glm::vec3 Camera::getFacing() {
    return facing;
}

Quaternion<float> Camera::getRotation() {
    return rotation;
}

void Camera::updateView() {

    std::array<float, 3> initFacingData = {0, 1, 0};
    std::array<float, 3> initUpData = {0, 0, 1};

    Vector<3, float> initFacing(initFacingData.data());
    Vector<3, float> initUp(initUpData.data());

    Matrix<3, 3, float> rotMat = this->rotation.toRotationMatrix();

    Vector<3, float> tmpFacing = rotMat * initFacing;
    Vector<3, float> up = rotMat * initUp;

    facing = glm::vec3(tmpFacing[0], tmpFacing[1], tmpFacing[2]);

    this->view = glm::lookAt(position, glm::vec3(0,0,0), glm::vec3(up[0], up[1], up[2]));

}

void Camera::updateProjection(float fov, float near, float far, float aspect) {

    this->fov = fov;
    this->near = near;
    this->far = far;
    this->aspect = aspect;

    this->projection = glm::perspective(fov, aspect, near, far);

}
