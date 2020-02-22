#ifndef SKELETALRIG_H
#define SKELETALRIG_H

#include <memory>
#include <vector>

#include <mathutils/quaternion.h>
#include <mathutils/vector.h>
#include <mathutils/matrix.h>

class Bone {

    public:
        Bone();

        Math::Matrix<4, 4, float> getTransformMatrix();

    private:
        std::vector<std::shared_ptr<Bone>> children;

        Math::Quaternion<float> rotation;
        Math::Vector<3, float>  offset;

};

class SkeletalRig
{
    public:
        SkeletalRig();
        virtual ~SkeletalRig();

    protected:

    private:

        std::shared_ptr<Bone> root;

};

#endif // SKELETALRIG_H
