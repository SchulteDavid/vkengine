#include "meshnode.h"

#include "render/renderelement.h"
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

MeshNode::~MeshNode() {

}

void MeshNode::addToWorld(std::shared_ptr<World> world) {

}

std::shared_ptr<Model> MeshNode::buildModel(vkutil::VulkanState & state) {

  if (this->model){
        return model;
  }

  unsigned int stride;
  lout << "Getting input elements" << std::endl;
  const std::vector<InputDescription> & elements = material->getShader()->getInputs();

  lout << "getting interleave data" << std::endl;
  std::vector<InterleaveElement> iData = mesh->compactStorage(elements, &stride);

  lout << "Saving mesh" << std::endl;
  mesh->saveAsPLY("structure.ply");

  lout << "contructing Model" << std::endl;
  this->model = std::shared_ptr<Model>(new Model(state, mesh, iData, stride));

  return model;

}

std::shared_ptr<Material> MeshNode::getMaterial() {
  return material;
}

std::shared_ptr<Mesh> MeshNode::getMesh() {
  return mesh;
}

void MeshNode::addToViewport(Viewport * view) {

  lout << "Adding MeshNode to Viewport" << std::endl;

  Transform<float> vTransform = convertTransform<double, float>(transform);
  lout << "Building RenderElement at position " << vTransform << std::endl;

  if (!this->renderElement) {
    std::shared_ptr<Model> tmpModel = buildModel(view->getState());
    renderElement = std::shared_ptr<RenderElement>(RenderElement::buildRenderElement(view, tmpModel, material, vTransform));
    view->addRenderElement(renderElement);
  }

  instance = renderElement->addInstance(vTransform);

}

void MeshNode::onTransformUpdate() {

  if (!this->renderElement) return;

  Transform<float> vTransform = convertTransform<double, float>(globalTransform);

  this->renderElement->updateInstance(instance, vTransform);
}

MeshNodeUploader::MeshNodeUploader(std::string nodeName, LoadingResource mesh, LoadingResource material, Transform<double> transform) : nodeName(nodeName) {
    this->meshRes = mesh;
    this->materialRes = material;
    this->transform = transform;
}

std::string MeshNodeUploader::getNodeName() {
  return nodeName;
}

std::shared_ptr<strc::Node> MeshNodeUploader::uploadResource() {

  std::shared_ptr<Mesh> mesh = std::dynamic_pointer_cast<Mesh>(meshRes->location);
  lout << "material location " << meshRes->location << std::endl;
  std::shared_ptr<Material> mat = std::dynamic_pointer_cast<Material>(materialRes->location);

  std::shared_ptr<strc::Node> node = std::make_shared<strc::MeshNode>(nodeName, mesh, mat, transform);

  populateChildren(node);

  return node;
}

bool MeshNodeUploader::uploadReady() {
  //lout << "Checking if MeshNode can be uploaded" << std::endl;
  return meshRes->status.isUseable && materialRes->status.isUseable && childrenReady();
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
