#ifndef SKELETALRIG_H
#define SKELETALRIG_H

#include <memory>
#include <vector>

#include <mathutils/quaternion.h>
#include <mathutils/vector.h>
#include <mathutils/matrix.h>

#include "resources/resource.h"
#include "resources/resourceuploader.h"
#include "resources/resourceloader.h"

#include "util/transform.h"

namespace strc {
  class Node;
}

struct Joint {

  Math::Matrix<4,4,float> inverseTransform;
  std::shared_ptr<strc::Node> node;

};

class Skin : public Resource {

public:
  Skin(std::vector<Joint> joints);
  virtual ~Skin();
  
  void writeTransformDataToBuffer(float * buffer);
  
  size_t getDataSize();
  Transform<double> getRootTransform();
  
private:
  
  std::vector<Joint> joints;

};

class SkinUploader : public ResourceUploader<Skin> {

public:

  SkinUploader();
  virtual ~SkinUploader();

  void addJoint(LoadingResource node, Math::Matrix<4, 4, float> inverse);
  
  std::shared_ptr<Skin> uploadResource(vkutil::VulkanState & state, ResourceManager * manager);
  bool uploadReady();

private:
  std::vector<LoadingResource> nodes;
  std::vector<Math::Matrix<4,4, float>> inverses;
  
};

#endif // SKELETALRIG_H
