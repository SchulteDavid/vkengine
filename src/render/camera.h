#ifndef CAMERA_H
#define CAMERA_H

#include <glm/mat4x4.hpp>
#include <mathutils/quaternion.h>

#include "util/transform.h"

class Camera {
 public:
  Camera(float fov, float near, float far, float aspect, glm::vec3 pos, Math::Quaternion<float> rotation = Math::Quaternion<float>(1,0,0,0));
  virtual ~Camera();

  void updateProjection(float nFov, float nNear, float nFar, float nAspect);

  glm::mat4 getProjection();
  glm::mat4 getView();

  void move(float dx, float dy, float dz);
  void move(Math::Vector<3, float> d);

  void setPosition(float x, float y, float z);

  void rotate(Math::Quaternion<float> dr);

  void setRotation(Math::Quaternion<float> r);

  Math::Vector<3, float> getFacing();
  Math::Quaternion<float> getRotation();
  Math::Vector<3, float> getPosition();

  Transform<float> getTransform();

  float getFov();
  float getAspect();

 protected:

  virtual void updateView();
  
 private:
  
  float fov;
  float near;
  float far;
  float aspect;

  glm::vec3 facing;

  Transform<float> transform;

  glm::mat4 projection;
  glm::mat4 view;

};

#endif // CAMERA_H
