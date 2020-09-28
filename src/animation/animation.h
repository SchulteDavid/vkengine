#ifndef ANIMATION_H
#define ANIMATION_H

#include <mathutils/quaternion.h>
#include <mathutils/vector.h>

#include "util/transform.h"

struct Keyframe {

  Transform<double> transform;

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
