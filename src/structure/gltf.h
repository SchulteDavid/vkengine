#ifndef GLTF_H
#define GLTF_H

#include <string>

#include "render/util/vkutil.h"
#include "util/mesh.h"
#include "util/meshhelper.h"
#include <mathutils/vector.h>
#include <mathutils/quaternion.h>
#include "animation/skeletalrig.h"
#include "animation/animation.h"
#include "node/node.h"

class GLTFNode {

public:
  GLTFNode();
  virtual ~GLTFNode();
  
  Math::Quaternion<float> getRotation();
  Math::Vector<3, float> getPosition();
  Math::Vector<3, float> getScale();

  Math::Matrix<4,4,float> getTransform();
  
  void setPosition(Math::Vector<3, float> pos);
  void setRotation(Math::Quaternion<float> rot);
  void setScale(Math::Vector<3, float> scale);
  
  void addChild(std::shared_ptr<GLTFNode> c);
  bool hasChildren();
  std::vector<std::shared_ptr<GLTFNode>> & getChildren();
  
  void setName(std::string name);
  std::string getName();
  
  void setMesh(std::shared_ptr<Mesh> mesh);
  std::shared_ptr<Mesh> getMesh();
  bool hasMesh();
  
  void setSkin(std::shared_ptr<Skin> skin);
  std::shared_ptr<Skin> getSkin();

private:

  std::string name;
  std::vector<std::shared_ptr<GLTFNode>> children;
  Math::Quaternion<float> rotation;
  Math::Vector<3, float>  position;
  Math::Vector<3, float> scale;

  std::shared_ptr<Mesh> mesh;
  std::shared_ptr<Skin> skin;
  std::unordered_map<std::string, std::shared_ptr<Animation>> animations;

};

struct gltf_file_data_t;

#include "resources/archiveloader.h"
#include "node/nodeloader.h"

struct gltf_loading_state_t;

class GLTFNodeLoader : public ArchiveLoader {

public:

  GLTFNodeLoader();
  
  //std::shared_ptr<ResourceUploader<strc::Node>> loadResource(std::string fname);
  bool canLoad(ResourceLocation location);
  LoadingResource load(ResourceLocation location);
  std::shared_ptr<NodeUploader> loadNodeGLTF(gltf_file_data_t & fileData, const int nodeId, const std::string fname, gltf_loading_state_t & state);

  LoadingResource loadMaterial(gltf_file_data_t & fileData, const int materialId, const std::string fname);
  LoadingResource loadTexture(gltf_file_data_t & fileData, const int textureId, const std::string fname);


  
};

struct gltf_file_data_t;
void gltfLoadFile(std::string fname, gltf_file_data_t * data = nullptr);

#endif // GLTF_H
