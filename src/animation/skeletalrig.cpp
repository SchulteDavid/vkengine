#include "skeletalrig.h"
#include "util/debug/trace_exception.h"
#include "util/debug/logger.h"

#include "string.h"

using namespace Math;
using namespace dbg;

#include "util/transform.h"
#include "node/node.h"

float zupData[16] = {
		     1, 0, 0, 0,
		     0, 0, -1, 0,
		     0, 1, 0, 0,
		     0, 0, 0, 1
};
Math::Matrix<4, 4, float> zupMatrix(zupData);

Math::Matrix<4,4,float> getTransformFromJoint(Joint & joint, Math::Matrix<4, 4, float> invParentTrans) {

  /// This should take less than a few hundred ns.
  //Transform<float> fTrans(Math::Vector<3, float>({0,0,1}), Math::Quaternion<float>(0.92388,0,-0.382683,0));
  Transform<float> fTrans = convertTransform<double,float>(joint.node->getGlobalTransform());
  //fTrans.position = jTrans.position;
  //fTrans.position = Math::Vector<3, float>();

  std::cout << fTrans.position << " " << fTrans.rotation << " " << fTrans.scale << std::endl;
  
  Math::Matrix<4, 4, float> tmat = getTransformationMatrix(fTrans);

  Math::Matrix<4, 4, float> invMat = zupMatrix * joint.inverseTransform;
  
  Math::Matrix<4, 4, float> res = invParentTrans * (tmat * invMat);

  std::cout << tmat << std::endl;
  
  return res;
  
  //return Math::Matrix<4, 4, float>();

}

Skin::Skin(std::vector<Joint> joints) : Resource("Skin") {
  this->joints = joints;
}

Skin::~Skin() {

}

Transform<double> Skin::getRootTransform() {
  return joints[0].node->getParentTransform();
}

void Skin::writeTransformDataToBuffer(float * buffer) {

  std::cout << "Skin has " << joints.size() << " joints: " << getDataSize() << std::endl;

  Transform<float> parentTransform = convertTransform<double, float>(joints[0].node->getParentTransform());
  Transform<float> invParentTrans = inverseTransform(parentTransform);

  Matrix<4, 4, float> invParentMat = getTransformationMatrix(invParentTrans);
  
  for (unsigned int i = 0; i < joints.size(); ++i) {

    Matrix<4,4,float> mat = getTransformFromJoint(joints[i], invParentMat);
    const float * arr = mat.asArray();

    void * dest = (void *) (buffer + 16 * i);
    std::cout << dest << std::endl;
    
    memcpy(dest, arr, 16 * sizeof(float));

  }

}

size_t Skin::getDataSize() {
  return 16 * sizeof(float) * joints.size();
}

SkinUploader::SkinUploader() {

}

SkinUploader::~SkinUploader () {

}

void SkinUploader::addJoint(LoadingResource node, Math::Matrix<4, 4, float> inverse) {

  nodes.push_back(node);
  inverses.push_back(inverse);
  
}

bool SkinUploader::uploadReady() {

  bool b = true;

  for (LoadingResource res : nodes) {
    b &= res->status.isUseable;
  }

  return b;
  
}

std::shared_ptr<Skin> SkinUploader::uploadResource(vkutil::VulkanState & state, ResourceManager * manager) {

  std::vector<Joint> joints(nodes.size());

  for (uint32_t i = 0; i < nodes.size(); ++i) {
    joints[i].node = std::dynamic_pointer_cast<strc::Node>(nodes[i]->location);
    joints[i].inverseTransform = inverses[i];
  }
  
  return std::make_shared<Skin>(joints);
  
}
