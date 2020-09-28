#include "meshnode.h"

#include "render/renderelement.h"
#include "render/viewport.h"

#include "nodeloader.h"

using namespace Math;
using namespace strc;

MeshNode::MeshNode(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, Transform<double> trans) : Node(trans) {

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

MeshNode::~MeshNode() {

}

void MeshNode::addToWorld(std::shared_ptr<World> world) {
  
}

std::shared_ptr<Model> MeshNode::buildModel(vkutil::VulkanState & state) {

  if (this->model){
        return model;
  }
  
  unsigned int stride;
  std::cout << "Getting input elements" << std::endl;
  const std::vector<InputDescription> & elements = material->getShader()->getInputs();

  std::cout << "getting interleave data" << std::endl;
  std::vector<InterleaveElement> iData = mesh->compactStorage(elements, &stride);

  std::cout << "Saving mesh" << std::endl;
  mesh->saveAsPLY("structure.ply");

  std::cout << "contructing Model" << std::endl;
  this->model = std::shared_ptr<Model>(new Model(state, mesh, iData, stride));
  
  return model;
  
}

void MeshNode::addToViewport(Viewport * view) {

  std::cout << "Adding MeshNode to Viewport" << std::endl;

  Transform<float> vTransform = convertTransform<double, float>(transform);
  std::cout << "Building RenderElement at position " << vTransform << std::endl;
  
  if (!this->renderElement) {
    std::shared_ptr<Model> tmpModel = buildModel(view->getState());
    renderElement = std::shared_ptr<RenderElement>(RenderElement::buildRenderElement(view, tmpModel, material, vTransform));
    view->addRenderElement(renderElement);
  }

  instance = renderElement->addInstance(vTransform);
  
}

void MeshNode::setTransform(Transform<double> trans) {

  if (!this->renderElement) return;

  Transform<float> vTransform = convertTransform<double, float>(globalTransform);

  this->renderElement->updateInstance(instance, vTransform);
}

MeshNodeUploader::MeshNodeUploader(LoadingResource mesh, LoadingResource material, Transform<double> transform) {
    this->meshRes = mesh;
    this->materialRes = material;
    this->transform = transform;
}

std::shared_ptr<strc::Node> MeshNodeUploader::uploadResource() {

  std::shared_ptr<Mesh> mesh = std::dynamic_pointer_cast<Mesh>(meshRes->location);
  std::shared_ptr<Material> mat = std::dynamic_pointer_cast<Material>(materialRes->location);
  
  std::shared_ptr<strc::Node> node = std::make_shared<strc::MeshNode>(mesh, mat, transform);
  
  populateChildren(node);
  
  return node;
}

bool MeshNodeUploader::uploadReady() {
  std::cout << "Checking if MeshNode can be uploaded" << std::endl;
  return meshRes->status.isUseable && materialRes->status.isUseable && childrenReady();
}

std::shared_ptr<NodeUploader> strc::loadMeshNode(std::shared_ptr<config::NodeCompound> root, const NodeLoader::LoadingContext & context) {

  LoadingResource mesh = nullptr;
  LoadingResource material = nullptr;
  
  if (root->hasChild("meshfile")) {
    std::string name(root->getNode<char>("meshfile")->getRawData());
    mesh = context.loader->loadDependencyResource("Mesh", name);
  }

  if (root->hasChild("matfile")) {
    std::string name(root->getNode<char>("matfile")->getRawData());
    material = context.loader->loadDependencyResource("Material", name);
  }

  return std::make_shared<MeshNodeUploader>(mesh, material, context.transform);
  
}
