#ifndef ANIMATION_H
#define ANIMATION_H

#include <mathutils/quaternion.h>
#include <mathutils/vector.h>

struct Keyframe {

    std::vector<Math::Quaternion<float>> rotations;
    std::vector<Math::Vector<3, float>> offsets;
    double t;

};

class Animation {

    public:
        Animation();
        virtual ~Animation();

    protected:

    private:

        std::vector<Keyframe> frames;

};

#endif // ANIMATION_H
