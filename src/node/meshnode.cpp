#include "meshnode.h"

#include "render/renderelement.h"
#include "render/renderelementanim.h"
#include "render/viewport.h"

#include "nodeloader.h"

using namespace Math;
using namespace strc;

MeshNode::MeshNode(std::string name, std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, Transform<double> trans) : Node(name, trans) {

  this->mesh = mesh;
  this->material = material;
  this->model = nullptr;

  if (!this->material) {
    throw dbg::trace_exception("Empty material in MeshNode");
  }

  if (!this->mesh) {
    throw dbg::trace_exception("Empty mesh in MeshNode");
  }

}

MeshNode::MeshNode(std::string name, std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, Transform<double> trans, std::shared_ptr<Skin> skin) : MeshNode(name, mesh, material, trans) {
  this->skin = skin;
}

MeshNode::~MeshNode() {

}

void MeshNode::addToWorld(World * world, std::shared_ptr<Node> self) {

}

std::shared_ptr<Node> MeshNode::duplicate(std::string name) {
  return std::make_shared<MeshNode>(name, mesh, material, transform, skin);
}

std::shared_ptr<Model> MeshNode::buildModel(vkutil::VulkanState & state) {

  if (this->model){
        return model;
  }

  if (skin && !material->getSkinShader())
    throw dbg::trace_exception("Skinned model has shader without skin inputs!");
  
  unsigned int stride;
  const std::vector<InputDescription> & elements = skin ? material->getSkinShader()->getInputs() : material->getShader()->getInputs();

  /*std::cout << "Mesh node '" << this->getName() << "' has the following descriptors:" << std::endl;
  for (const InputDescription & idesc : elements) {
    std::cout << idesc.attributeName << " at " << idesc.location << std::endl;
    }*/
  
  std::vector<InterleaveElement> iData = mesh->compactStorage(elements, &stride);

  this->model = std::shared_ptr<Model>(new Model(state, mesh, iData, stride));

  return model;

}

std::shared_ptr<Material> MeshNode::getMaterial() {
  return material;
}

std::shared_ptr<Mesh> MeshNode::getMesh() {
  return mesh;
}

void MeshNode::addToViewport(Viewport * view, std::shared_ptr<Node> self) {

  Transform<float> vTransform = convertTransform<double, float>(getGlobalTransform());

  if (!this->renderElement) {
    std::shared_ptr<Model> tmpModel = buildModel(view->getState());
    //renderElement = std::shared_ptr<RenderElement>(RenderElement::buildRenderElement(view, tmpModel, material, vTransform));
    if (this->skin) {

      std::cout << "Adding Animation to viewport" << std::endl;
      renderElement = std::make_shared<RenderElementAnim>(view, tmpModel, material, vTransform, skin);
      
    } else {
      
      renderElement = std::make_shared<RenderElement>(view, tmpModel, material, view->getSwapchainSize(), vTransform);
      
    }
    renderElement->constructBuffers(view->getSwapchainSize());
    view->addRenderElement(renderElement);
  }

  instance = renderElement->addInstance(vTransform);

}

void MeshNode::onTransformUpdate() {

  if (!this->renderElement) return;

  Transform<float> vTransform = convertTransform<double, float>(globalTransform);

  this->renderElement->updateInstance(instance, vTransform);
}

void MeshNode::onUpdate(const double dt, const double t) {

  if (this->skin) {

    
    
  }
  
}

MeshNodeUploader::MeshNodeUploader(std::string nodeName, LoadingResource mesh, LoadingResource material, Transform<double> transform) : nodeName(nodeName) {
    this->meshRes = mesh;
    this->materialRes = material;
    this->transform = transform;
}

std::string MeshNodeUploader::getNodeName() {
  return nodeName;
}

std::shared_ptr<strc::Node> MeshNodeUploader::constructNode() {

  std::shared_ptr<Mesh> mesh = std::dynamic_pointer_cast<Mesh>(meshRes->location);
  std::shared_ptr<Material> mat = std::dynamic_pointer_cast<Material>(materialRes->location);

  std::shared_ptr<strc::Node> node;
  if (this->skinResource) {
    std::shared_ptr<Skin> skin = std::dynamic_pointer_cast<Skin>(skinResource->location);
    node = std::make_shared<strc::MeshNode>(nodeName, mesh, mat, transform, skin);
  } else {
    node = std::make_shared<strc::MeshNode>(nodeName, mesh, mat, transform);
  }

  return node;
}

bool MeshNodeUploader::uploadReady() {
  //lout << "Checking if MeshNode can be uploaded" << std::endl;
  return meshRes->status.isUseable && materialRes->status.isUseable && childrenReady() && (!skinResource || skinResource->status.isUseable);
}

void MeshNodeUploader::addSkin(LoadingResource res) {
  this->skinResource = res;
}

std::shared_ptr<NodeUploader> strc::loadMeshNode(std::shared_ptr<config::NodeCompound> root, const NodeLoader::LoadingContext & context, const std::string nodeName) {

  LoadingResource mesh = nullptr;
  LoadingResource material = nullptr;

  if (root->hasChild("mesh")) {
    std::string name(root->getNode<char>("mesh")->getRawData());
    ResourceLocation location = ResourceLocation::parse("Mesh", name);
    mesh = context.loader->loadDependencyResource(location);
    //mesh = context.loader->loadDependencyResource(ResourceLocation("Mesh", name));
  }

  if (root->hasChild("material")) {
    std::string name(root->getNode<char>("material")->getRawData());
    ResourceLocation location = ResourceLocation::parse("Material", name);
    material = context.loader->loadDependencyResource(location);
  }

  return std::make_shared<MeshNodeUploader>(nodeName, mesh, material, context.transform);

}
