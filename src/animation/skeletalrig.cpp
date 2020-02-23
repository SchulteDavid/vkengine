#include "skeletalrig.h"
#include "util/debug/trace_exception.h"

#include "string.h"

using namespace Math;
using namespace dbg;

Math::Matrix<4,4,float> getTransformFromJoint(Joint & joint) {

    /// This should take less than a few hundred ns.
    return joint.inverseTransform * joint.rotation.toModelMatrix(joint.offset);

}

Skin::Skin(std::vector<Joint> joints) {
    this->joints = joints;
}

Skin::~Skin() {

}

void Skin::writeTransformDataToBuffer(float * buffer) {

    for (unsigned int i = 0; i < joints.size(); ++i) {

        Matrix<4,4,float> mat = getTransformFromJoint(joints[i]);
        memcpy(buffer + 16 * i, mat.asArray(), 16 * sizeof(float));

    }

}
