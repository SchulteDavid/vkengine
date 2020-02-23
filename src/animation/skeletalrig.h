#ifndef SKELETALRIG_H
#define SKELETALRIG_H

#include <memory>
#include <vector>

#include <mathutils/quaternion.h>
#include <mathutils/vector.h>
#include <mathutils/matrix.h>

struct Joint {

    Math::Matrix<4,4,float> inverseTransform;
    Math::Vector<3,float> offset;
    Math::Quaternion<float> rotation;

};

class Skin {

    public:
        Skin(std::vector<Joint> joints);
        virtual ~Skin();

        void writeTransformDataToBuffer(float * buffer);

    private:

        std::vector<Joint> joints;

};

#endif // SKELETALRIG_H
