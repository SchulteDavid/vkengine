#include "node/physicsnode.h"

#include "world/world.h"

using namespace strc;

PhysicsNode::PhysicsNode(std::string name, Transform<double> trans, std::shared_ptr<PhysicsObject> obj) : Node(name, trans) {

  this->physObject = obj;
  isInSimulation = false;

}

PhysicsNode::~PhysicsNode() {

}

std::shared_ptr<NodeUploader> strc::loadPhysicsNode(std::shared_ptr<config::NodeCompound> root, const NodeLoader::LoadingContext & context, const std::string nodeName) {

  std::cout << "Loading PhysicsNode" << std::endl;

  std::shared_ptr<config::NodeCompound> objectNode = root->getNodeCompound("physics");

  std::shared_ptr<PhysicsObject> object = physutil::loadPhysicsObject(objectNode, context.transform);

  std::shared_ptr<Node> node(new PhysicsNode(nodeName, context.transform, object));

  return std::make_shared<NodeUploader>(node);
  
}

std::shared_ptr<Node> PhysicsNode::duplicate(std::string name) {
  std::shared_ptr<PhysicsObject> nObject(new PhysicsObject(this->physObject->getMass(), transform, this->physObject->getCollisionShape()));
  std::shared_ptr<PhysicsNode> res = std::make_shared<PhysicsNode>(name, transform, nObject);
  return res;
}

void PhysicsNode::onTransformUpdate() {
  if (!isInSimulation)
    this->physObject->setTransform(getGlobalTransform());
}

void PhysicsNode::applyImpulse(Math::Vector<3> impulse, Math::Vector<3> position) {
  this->physObject->applyImpulse(impulse);
}

void PhysicsNode::addToWorld(World * world, std::shared_ptr<Node> self) {

  isInSimulation = true;
  this->world = world;
  
}

void PhysicsNode::onUpdate(const double dt, const double t) {

  double dist;
  std::shared_ptr<strc::Node> node = this->world->raycast(physObject->getPosition(), Math::Vector<3>({0,0,-1}), 1.0, dist);
  if (node) {
    std::cout << "Node "<< this << " hits node " << node << " dist = " << dist << std::endl;
  }
  
}

std::shared_ptr<PhysicsObject> PhysicsNode::getPhysicsObject() {
  return physObject;
}

void PhysicsNode::synchronize() {

  Transform<double> trans = physObject->getTransform();
  setGlobalTransform(trans);
  
}
