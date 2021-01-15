#include "node.h"

#include <functional>
#include "nodeloader.h"
#include "node/event.h"

#include "animation/animationplayer.h"

using namespace Math;
using namespace strc;

Node::Node(std::string name) : Node(name, Transform<double>()) {
}

Node::Node(std::string name, Transform<double> transform) : name(name) {

  this->transform = transform;
  this->globalTransform = transform;
  this->parentTransform = transform;

  this->eventHandler = std::make_shared<EventHandler>();
  this->animationPlayer = nullptr;

}

Node::~Node() {

}

void Node::attachEventHandler(std::shared_ptr<EventHandler> handler, std::shared_ptr<Node> self) {
  this->eventHandler = handler;
  eventHandler->bindToParent(self);
}

void Node::attachResource(std::string name, std::shared_ptr<Resource> resource) {
  attachedResources[name] = resource;
}

std::shared_ptr<Resource> Node::getAttachedResource(std::string name) {
  if (attachedResources.find(name) == attachedResources.end()) {
    throw dbg::trace_exception(std::string("No such attached Resource ").append(name));
  }
  return attachedResources[name];
}

void Node::addChild(std::shared_ptr<Node> child) {
 
  
  if (!child) {
    throw dbg::trace_exception("Trying to add null-child to node");
  }
  this->children[child->getName()] = child;

  child->setTransform(child->getTransform(), globalTransform);
  
}

std::shared_ptr<Node> Node::getChild(std::string name) {
  if (children.find(name) == children.end()) {
    throw dbg::trace_exception(std::string("No child named ").append(name));
  }
  return children[name];
}

void Node::viewportAdd(Viewport * view, std::shared_ptr<Node> self) {

  this->addToViewport(view, self);

  for (auto n : this->children) {
    n.second->viewportAdd(view, n.second);
  }

}

void Node::worldAdd(std::shared_ptr<World> world, std::shared_ptr<Node> self) {

  this->addToWorld(world, self);

  for (auto n : children) {
    n.second->worldAdd(world, n.second);
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
  this->globalTransform = this->parentTransform * trans;

  this->onTransformUpdate();

  for (auto child : children) {
    child.second->setTransform(child.second->getTransform(), globalTransform);
  }

}
void Node::setTransform(Transform<double> trans, Transform<double> ptrans) {

  this->transform = trans;
  this->parentTransform = ptrans;
  this->globalTransform = ptrans * trans;

  this->onTransformUpdate();

  for (auto child : children) {
    child.second->setTransform(child.second->getTransform(), globalTransform);
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

const Transform<double> & Node::getParentTransform() {
  return parentTransform;
}

const std::string Node::getName() {
  return name;
}

void Node::onUpdate(const double dt, const double t) {
  
}

void Node::update(const double dt, const double t) {

  if (animationPlayer) {
    std::cout << "Animating node: " << name << std::endl;
    animationPlayer->applyToNode(t, *this);
  }
  
  this->eventHandler->onUpdate(dt, t);
  
  for (auto child : children) {
    child.second->update(dt, t);
  }

  onUpdate(dt, t);
  
}

void Node::addAnimation(std::string name, std::shared_ptr<Animation> animation) {

  if (!this->animationPlayer) {
    this->animationPlayer = std::make_shared<AnimationPlayer>();
  }

  this->animationPlayer->addAnimation(name, animation);

  animationPlayer->plotAnimPath(name, this->name);
  
}

#include "meshnode.h"
#include "lightnode.h"
#include "physicsnode.h"
#include "audiosourcenode.h"

void Node::registerLoaders() {

  NodeLoader::registerNodeLoader("Node", loadDefaultNode);
  NodeLoader::registerNodeLoader("MeshNode", strc::loadMeshNode);
  NodeLoader::registerNodeLoader("LightNode", strc::loadLightNode);
  NodeLoader::registerNodeLoader("PhysicsNode", strc::loadPhysicsNode);
  NodeLoader::registerNodeLoader("AudioSourceNode", strc::loadAudioSourceNode);

  /// Register EventHandlers

  registerEventHandlerType("EventHandler", [] {return new EventHandler();});
  
}

std::shared_ptr<Node> Node::duplicate(std::string name) {
  return std::make_shared<Node>(name, transform);
}

std::shared_ptr<Node> Node::createDuplicate(std::string name) {

  std::shared_ptr<Node> res = duplicate(name);

  for (auto c : children) {
    std::shared_ptr<Node> nChild(c.second->duplicate(c.first));
    res->addChild(nChild);
  }

  for (auto it : attachedResources) {
    res->attachResource(it.first, it.second);
  }

  res->attachEventHandler(eventHandler, res);

  if (animationPlayer) {
    res->animationPlayer = animationPlayer;
  }

  return res;
}
