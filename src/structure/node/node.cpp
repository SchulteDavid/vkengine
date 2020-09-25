#include "node.h"

#include <functional>
#include "nodeloader.h"

using namespace Math;
using namespace strc;

Node::Node() : Node(Transform<double>()) {

}

Node::Node(Transform<double> transform) {

  this->transform = transform;
  
}

Node::~Node() {

}

void Node::addChild(std::shared_ptr<Node> child) {
  if (!child) {
    throw dbg::trace_exception("Trying to add null-child to node");
  }
  this->children.push_back(child);
}

const std::vector<std::shared_ptr<Node>> & Node::getChildren() {
  return children;
}

void Node::viewportAdd(Viewport * view) {

  this->addToViewport(view);

  for (std::shared_ptr<Node> n : this->children) {
    n->viewportAdd(view);
  }
  
}

void Node::addToWorld(std::shared_ptr<World> world) {
  
}

void Node::addToViewport(Viewport * view) {

}

std::shared_ptr<NodeUploader> loadDefaultNode(std::shared_ptr<config::NodeCompound> root, const NodeLoader::LoadingContext & context) {

  std::shared_ptr<strc::Node> node = std::make_shared<strc::Node>(context.transform);
  
  return std::make_shared<NodeUploader>(node);
  
}

#include "meshnode.h"
#include "lightnode.h"

void Node::registerLoaders() {

  NodeLoader::registerNodeLoader("Node", loadDefaultNode);
  NodeLoader::registerNodeLoader("MeshNode", strc::loadMeshNode);
  NodeLoader::registerNodeLoader("LightNode", strc::loadLightNode);
  
}
