#include "node.h"

#include <functional>
#include "nodeloader.h"
#include "node/event.h"

#include "world/world.h"

#include "animation/animationplayer.h"

using namespace Math;
using namespace strc;

Node::Node(std::string name) : Node(name, Transform<double>()) {
}

Node::Node(std::string name, Transform<double> transform) : Resource("Node"), name(name) {

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

void Node::attachResource(std::string name, std::shared_ptr<Resource> res) {

  attachedResources[name] = res;
  
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

void Node::worldAdd(World * world, std::shared_ptr<Node> self) {

  this->addToWorld(world, self);

  for (auto n : children) {
    //n.second->worldAdd(world, n.second);
    world->addNode(n.second);
  }
  
}

void Node::addToWorld(World * world, std::shared_ptr<Node> self) {

}

void Node::addToViewport(Viewport * view, std::shared_ptr<Node> self) {

}

void Node::synchronize() {

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

void Node::setGlobalTransform(Transform<double> trans) {

  this->transform = trans;
  this->globalTransform = trans;
  this->parentTransform = Transform<double>();

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

std::shared_ptr<config::NodeCompound> transformToCompound(const Transform<double> & trans) {

  std::shared_ptr<config::NodeCompound> tNode = std::make_shared<config::NodeCompound>();
  
  double tmp[4];

  tmp[0] = trans.position(0);
  tmp[1] = trans.position(1);
  tmp[2] = trans.position(2);

  tNode->addChild("position", std::make_shared<config::Node<double>>(3, tmp));

  tmp[0] = trans.rotation.a;
  tmp[1] = trans.rotation.b;
  tmp[2] = trans.rotation.c;
  tmp[3] = trans.rotation.d;

  tNode->addChild("rotation", std::make_shared<config::Node<double>>(4, tmp));

  tmp[0] = trans.scale(0);
  tmp[1] = trans.scale(1);
  tmp[2] = trans.scale(2);

  tNode->addChild("scale", std::make_shared<config::Node<double>>(3, tmp));

  return tNode;
    
  
}

void Node::saveNode(std::shared_ptr<config::NodeCompound> comp) {

  
  
}

std::string Node::getTypeName() {

  int status;
  char * demangled = abi::__cxa_demangle(typeid(*this).name(), 0, 0, &status);

  char * name = demangled + 6;
  std::string res = std::string(name);
  free(demangled);
  return res;
  
}

std::shared_ptr<config::NodeCompound> Node::toCompoundNode() {

  std::shared_ptr<config::NodeCompound> root = std::make_shared<config::NodeCompound>();

  std::string tName = getTypeName();
  root->addChild("type", std::make_shared<config::Node<char>>(tName.length(), tName.c_str()));
  root->addChild("name", std::make_shared<config::Node<char>>(name.length(), name.c_str()));
  root->addChild("transform", transformToCompound(transform));
  
  saveNode(root);

  if (attachedResources.size()) {

    std::vector<std::shared_ptr<config::NodeCompound>> resComps;
    
    for (auto it : attachedResources) {

      std::shared_ptr<config::NodeCompound> resComp = std::make_shared<config::NodeCompound>();

      const ResourceLocation & location = it.second->getLocation();
      
      resComp->addChild("name", std::make_shared<config::Node<char>>(it.first.length(), it.first.c_str()));
      resComp->addChild("type", std::make_shared<config::Node<char>>(location.type.length(), location.type.c_str()));
      std::string loc = location;
      resComp->addChild("location", std::make_shared<config::Node<char>>(loc.length(), loc.c_str()));

      resComps.push_back(resComp);
      
    }

    root->addChild("attachedResources", std::make_shared<config::Node<std::shared_ptr<config::NodeCompound>>>(resComps.size(), resComps.data()));
    
  }
  
  if (children.size()) {

    std::vector<std::shared_ptr<config::NodeCompound>> childComps;
    //uint32_t i;
    
    for (auto it = children.begin(); it != children.end(); ++it) {

      if (it->second)
	childComps.push_back(it->second->toCompoundNode());
      
    }

    root->addChild("children", std::make_shared<config::Node<std::shared_ptr<config::NodeCompound>>>(childComps.size(), childComps.data()));
    
  }

  return root;
  
}

void Node::printChildren() {

  for (auto it : children) {
    std::cout << it.first << std::endl;
  }
  
}

std::unordered_map<std::string, std::shared_ptr<strc::Node>> & Node::getChildren() {
  return children;
}

#include "meshnode.h"
#include "lightnode.h"
#include "physicsnode.h"
#include "audiosourcenode.h"

template <class T> std::shared_ptr<NodeUploader> loadNode(std::shared_ptr<config::NodeCompound> root, const NodeLoader::LoadingContext & context, const std::string nodeName) {
  
  int status;
  char * demangled = abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);

  std::string tName(demangled);
  free(demangled);
  
  throw dbg::trace_exception(std::string("No specialisation for node type: ").append(tName));
}

template <> std::shared_ptr<NodeUploader> loadNode<strc::Node>(std::shared_ptr<config::NodeCompound> root, const NodeLoader::LoadingContext & context, const std::string nodeName) {
  return loadDefaultNode(root, context, nodeName);
}

template <> std::shared_ptr<NodeUploader> loadNode<strc::MeshNode>(std::shared_ptr<config::NodeCompound> root, const NodeLoader::LoadingContext & context, const std::string nodeName) {
  return loadMeshNode(root, context, nodeName);
}

template <> std::shared_ptr<NodeUploader> loadNode<strc::PhysicsNode>(std::shared_ptr<config::NodeCompound> root, const NodeLoader::LoadingContext & context, const std::string nodeName) {
  return loadPhysicsNode(root, context, nodeName);
}

template <> std::shared_ptr<NodeUploader> loadNode<strc::AudioSourceNode>(std::shared_ptr<config::NodeCompound> root, const NodeLoader::LoadingContext & context, const std::string nodeName) {
  return loadAudioSourceNode(root, context, nodeName);
}

template <> std::shared_ptr<NodeUploader> loadNode<strc::LightNode>(std::shared_ptr<config::NodeCompound> root, const NodeLoader::LoadingContext & context, const std::string nodeName) {
  return loadLightNode(root, context, nodeName);
}

template <class T> void registerLoader() {

  int status;
  char * demangled = abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);

  char * name = demangled + 6;
  std::string res = std::string(name);
  free(demangled);

  NodeLoader::registerNodeLoader(res, loadNode<T>);
  
}

void Node::registerLoaders() {

  registerLoader<Node>();
  registerLoader<MeshNode>();
  registerLoader<PhysicsNode>();
  registerLoader<AudioSourceNode>();
  registerLoader<LightNode>();

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
