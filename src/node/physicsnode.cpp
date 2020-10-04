#include "node/physicsnode.h"

#include "world/entity.h"
#include "world/world.h"

using namespace strc;

PhysicsNode::PhysicsNode(std::string name, Transform<double> trans, std::shared_ptr<PhysicsObject> obj) : Node(name, trans) {

  this->physObject = obj;

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

void PhysicsNode::applyImpulse(Math::Vector<3> impulse, Math::Vector<3> position) {
  this->physObject->applyImpulse(impulse);
}

void PhysicsNode::addToWorld(std::shared_ptr<World> world, std::shared_ptr<Node> self) {

  std::shared_ptr<Entity> ent = Entity::buildEntityFromType("Entity", self, physObject);
  std::cout << "Adding entity to world" << ent << std::endl;
  world->addEntity(ent);
  
}
