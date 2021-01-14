#ifndef MESH_NODE_H
#define MESH_NODE_H

#include "node.h"

#include "util/mesh.h"
#include "render/material.h"
#include "render/renderelement.h"

#include "nodeloader.h"

#include "animation/skeletalrig.h"

namespace strc {

  class MeshNode : public Node {

  public:
    MeshNode(std::string name, std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, Transform<double> trans);
    MeshNode(std::string name, std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, Transform<double> trans, std::shared_ptr<Skin> skin);

    virtual ~MeshNode();

    std::shared_ptr<Model> buildModel(vkutil::VulkanState & state);
    std::shared_ptr<Material> getMaterial();
    std::shared_ptr<Mesh> getMesh();

  protected:


    void addToWorld(std::shared_ptr<World> world, std::shared_ptr<Node> self) override;
    void addToViewport(Viewport * view, std::shared_ptr<Node> self) override;

    void onUpdate(const double dt, const double t) override;

    void onTransformUpdate() override;

    std::shared_ptr<Node> duplicate(std::string name) override;

  private:
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Model> model;
    std::shared_ptr<Material> material;
    std::shared_ptr<RenderElement> renderElement;

    std::shared_ptr<Skin> skin;

    RenderElement::Instance instance;

  };

  std::shared_ptr<NodeUploader> loadMeshNode(std::shared_ptr<config::NodeCompound> root, const NodeLoader::LoadingContext & context, const std::string nodeName);

}

class MeshNodeUploader : public NodeUploader {

public:

  MeshNodeUploader(std::string nodeName, LoadingResource mesh, LoadingResource material, Transform<double> transform);

  bool uploadReady() override;

  std::string getNodeName() override;

  void addSkin(LoadingResource res);

protected:
  std::shared_ptr<strc::Node> constructNode() override;

private:

  LoadingResource meshRes;
  LoadingResource materialRes;
  Transform<double> transform;

  LoadingResource skinResource;
  
  const std::string nodeName;

};

#endif
