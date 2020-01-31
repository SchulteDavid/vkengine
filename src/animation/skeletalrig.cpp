#include "skeletalrig.h"

using namespace Math;
using namespace dbg;

Matrix<4, 4, float> Bone::getTransformMatrix() {

    return rotation.toModelMatrix(offset);

}

SkeletalRig::SkeletalRig()
{
    //ctor
}

SkeletalRig::~SkeletalRig()
{
    //dtor
}
