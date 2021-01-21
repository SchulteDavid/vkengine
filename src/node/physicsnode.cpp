#include "node/physicsnode.h"

#include "world/world.h"

#include "node/nodeloader.h"

using namespace strc;

PhysicsNode::PhysicsNode(std::string name, Transform<double> trans, std::shared_ptr<PhysicsObject> obj) : Node(name, trans) {

  this->physObject = obj;
  isInSimulation = false;

}

PhysicsNode::~PhysicsNode() {

}

class PhysicsNodeUploader : public NodeUploader {

public:
  PhysicsNodeUploader(std::string name, Transform<double> transform, std::shared_ptr<config::NodeCompound> pData) : NodeUploader(), nodeName(name) {

    this->transform = transform;
    this->physicsData = pData;
    
  }

  virtual ~PhysicsNodeUploader() {

  }

  std::shared_ptr<Node> constructNode() override {

    std::shared_ptr<PhysicsObject> pobj = physutil::loadPhysicsObject(physicsData, transform, this->getAttachedResources());

    return std::make_shared<PhysicsNode>(nodeName, transform, pobj);
    
  }

private:

  std::string nodeName;
  Transform<double> transform;
  std::shared_ptr<config::NodeCompound> physicsData;
  
};

std::shared_ptr<NodeUploader> strc::loadPhysicsNode(std::shared_ptr<config::NodeCompound> root, const NodeLoader::LoadingContext & context, const std::string nodeName) {

  /*std::cout << "Loading PhysicsNode" << std::endl;

  std::shared_ptr<config::NodeCompound> objectNode = root->getNodeCompound("physics");

  std::shared_ptr<PhysicsObject> object = physutil::loadPhysicsObject(objectNode, context.transform);

  std::shared_ptr<Node> node(new PhysicsNode(nodeName, context.transform, object));

  return std::make_shared<NodeUploader>(node);*/

  return std::make_shared<PhysicsNodeUploader>(nodeName, context.transform, root->getNodeCompound("physics"));
  
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

void PhysicsNode::applyImpulse(const Math::Vector<3> & impulse, const Math::Vector<3> & position) {
  this->physObject->applyImpulse(impulse, position);
}

void PhysicsNode::addToWorld(World * world, std::shared_ptr<Node> self) {

  isInSimulation = true;
  this->world = world;
  
}

void PhysicsNode::onUpdate(const double dt, const double t) {

}

std::shared_ptr<PhysicsObject> PhysicsNode::getPhysicsObject() {
  return physObject;
}

void PhysicsNode::synchronize() {

  Transform<double> trans = physObject->getTransform();
  setGlobalTransform(trans);
  
}

World * PhysicsNode::getWorld() {
  return world;
}

void PhysicsNode::saveNode(std::shared_ptr<config::NodeCompound> comp) {

  std::shared_ptr<config::NodeCompound> physData = physutil::savePhysicsObject(physObject);

  comp->addChild("physics", physData);  
}
