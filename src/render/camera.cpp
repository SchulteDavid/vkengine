#include "camera.h"

#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

extern glm::mat4 toGLMMatrix(Math::Matrix<4, 4, float> mat);

using namespace Math;

Camera::Camera(float fov, float near, float far, float aspect, glm::vec3 pos, Math::Quaternion<float> rot) {

    this->updateProjection(fov, near, far, aspect);

    this->transform.position = Math::Vector<3, float>(pos.x, pos.y, pos.z);
    this->transform.rotation = rot;
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

    transform.position = transform.position + Vector<3, float>(dx, dy, dz);

    updateView();

}

Transform<float> Camera::getTransform() {
  return transform;
}

void Camera::move(Math::Vector<3, float> d) {
    transform.position = transform.position + d;
    updateView();
}

void Camera::setPosition(float x, float y, float z) {

  transform.position = Vector<3, float>(x,y,z);

  updateView();

}

void Camera::setRotation(Math::Quaternion<float> r) {
  transform.rotation = r;
  updateView();
}

void Camera::rotate(Math::Quaternion<float> d) {

    transform.rotation = d * transform.rotation;
    updateView();

}

glm::vec3 Camera::getFacing() {
    return facing;
}

Quaternion<float> Camera::getRotation() {
    return transform.rotation;
}

Math::Vector<3, float> Camera::getPosition() {
  return transform.position;
}

void Camera::updateView() {
  
  Transform<float> inverse = inverseTransform(transform);
  view = toGLMMatrix(getTransformationMatrix(inverse));

}

void Camera::updateProjection(float fov, float near, float far, float aspect) {

    this->fov = fov;
    this->near = near;
    this->far = far;
    this->aspect = aspect;

    this->projection = glm::perspective(fov, aspect, near, far);

}
