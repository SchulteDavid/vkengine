#include "node.h"

#include <functional>
#include "nodeloader.h"

using namespace Math;
using namespace strc;

Node::Node(std::string name) : Node(name, Transform<double>()) {

}

Node::Node(std::string name, Transform<double> transform) : name(name) {

  this->transform = transform;
  this->globalTransform = transform;

}

Node::~Node() {

}

void Node::addChild(std::shared_ptr<Node> child) {
 
  std::cout << "Adding child to node " << this->name << " : " << child->name << std::endl;
  
  if (!child) {
    throw dbg::trace_exception("Trying to add null-child to node");
  }
  this->children.push_back(child);
  std::cout << "Child in vector" << std::endl;

  child->setTransform(child->getTransform(), globalTransform);

  std::cout << "Child added and transform set" << std::endl;
  
}

const std::vector<std::shared_ptr<Node>> & Node::getChildren() {
  return children;
}

void Node::viewportAdd(Viewport * view, std::shared_ptr<Node> self) {

  this->addToViewport(view, self);

  for (std::shared_ptr<Node> n : this->children) {
    n->viewportAdd(view, n);
  }

}

void Node::worldAdd(std::shared_ptr<World> world, std::shared_ptr<Node> self) {

  this->addToWorld(world, self);

  for (std::shared_ptr<Node> n : children) {
    n->worldAdd(world, n);
  }
  
}

void Node::addToWorld(std::shared_ptr<World> world, std::shared_ptr<Node> self) {

}

void Node::addToViewport(Viewport * view, std::shared_ptr<Node> self) {

}

std::shared_ptr<NodeUploader> loadDefaultNode(std::shared_ptr<config::NodeCompound> root, const NodeLoader::LoadingContext & context, const std::string nodeName) {

  std::shared_ptr<strc::Node> node = std::make_shared<strc::Node>(nodeName, context.transform);

  return std::make_shared<NodeUploader>(node);

}

void Node::setTransform(Transform<double> trans) {

  this->transform = trans;
  this->globalTransform = trans;

  this->onTransformUpdate();

  for (std::shared_ptr<Node> child : children) {
    child->setTransform(child->getTransform(), globalTransform);
  }

}
void Node::setTransform(Transform<double> trans, Transform<double> ptrans) {

  this->transform = trans;
  this->globalTransform = ptrans * trans;

  this->onTransformUpdate();

  for (std::shared_ptr<Node> child : children) {
    child->setTransform(child->getTransform(), globalTransform);
  }

}

void Node::onTransformUpdate() {

}

const Transform<double> & Node::getTransform() {
  return transform;
}

const Transform<double> & Node::getGlobalTransform() {
  return globalTransform;
}

const std::string Node::getName() {
  return name;
}

#include "meshnode.h"
#include "lightnode.h"
#include "physicsnode.h"

void Node::registerLoaders() {

  NodeLoader::registerNodeLoader("Node", loadDefaultNode);
  NodeLoader::registerNodeLoader("MeshNode", strc::loadMeshNode);
  NodeLoader::registerNodeLoader("LightNode", strc::loadLightNode);
  NodeLoader::registerNodeLoader("PhysicsNode", strc::loadPhysicsNode);

}
