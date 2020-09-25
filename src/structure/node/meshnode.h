#ifndef MESH_NODE_H
#define MESH_NODE_H

#include "node.h"

#include "util/mesh.h"
#include "render/material.h"

#include "nodeloader.h"

namespace strc {

  class MeshNode : public Node {
    
  public:
    MeshNode(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, Transform<double> trans);
    
    virtual ~MeshNode();
    
  protected:

    std::shared_ptr<Model> buildModel(vkutil::VulkanState & state);

    void addToWorld(std::shared_ptr<World> world) override;
    void addToViewport(Viewport * view) override;
    
  private:
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Model> model;
    std::shared_ptr<Material> material;
  
  };

  std::shared_ptr<NodeUploader> loadMeshNode(std::shared_ptr<config::NodeCompound> root, const NodeLoader::LoadingContext & context);
  
}

class MeshNodeUploader : public NodeUploader {

public:

  MeshNodeUploader(LoadingResource mesh, LoadingResource material, Transform<double> transform);

  bool uploadReady() override;

  std::shared_ptr<strc::Node> uploadResource() override;
  
private:

  LoadingResource meshRes;
  LoadingResource materialRes;
  Transform<double> transform;
  
};

#endif
