#include "skeletalrig.h"
#include "util/debug/trace_exception.h"

#include "string.h"

using namespace Math;
using namespace dbg;

Math::Matrix<4,4,float> getTransformFromJoint(Joint & joint) {

    /// This should take less than a few hundred ns.
    return joint.rotation.toModelMatrix(joint.offset) * joint.inverseTransform;
    //return joint.inverseTransform;

}

Skin::Skin(std::vector<Joint> joints) {
    this->joints = joints;
    for (Joint & j : this->joints) {

        std::cout << "Before transpose " << std::endl << j.inverseTransform << std::endl;
        j.inverseTransform = j.inverseTransform.transpose();
        std::cout << "After transpose " << std::endl << j.inverseTransform << std::endl;

    }
}

Skin::~Skin() {

}

void Skin::writeTransformDataToBuffer(float * buffer) {

    for (unsigned int i = 0; i < joints.size(); ++i) {

        Matrix<4,4,float> mat = getTransformFromJoint(joints[i]);
        //std::cout << mat << std::endl;
        const float * arr = mat.asArray();

        //std::cout << "arr[0] = " << arr[0] << " arr[4] = " << arr[4] << std::endl;

        memcpy(buffer + 16 * i, arr, 16 * sizeof(float));

    }

}

size_t Skin::getDataSize() {
    return 16 * sizeof(float) * joints.size();
}
