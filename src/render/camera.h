#ifndef CAMERA_H
#define CAMERA_H

#include <glm/mat4x4.hpp>
#include "../util/math/quaternion.h"

class Camera
{
    public:
        Camera(float fov, float near, float far, float aspect, glm::vec3 pos, Math::Quaternion<float> rotation = Math::Quaternion<float>(1,0,0,0));
        virtual ~Camera();

        void updateProjection(float nFov, float nNear, float nFar, float nAspect);

        glm::mat4 getProjection();
        glm::mat4 getView();

        void move(float dx, float dy, float dz);
        void move(glm::vec3 d);

        void rotate(Math::Quaternion<float> dr);
        void rotate(float dPitch, float dYaw);

        glm::vec3 position;

        glm::vec3 getFacing();
        Math::Quaternion<float> getRotation();


    protected:

        virtual void updateView();

    private:

        float fov;
        float near;
        float far;
        float aspect;

        float pitch;
        float yaw;

        glm::vec3 facing;
        Math::Quaternion<float> rotation;

        glm::mat4 projection;
        glm::mat4 view;

};

#endif // CAMERA_H
