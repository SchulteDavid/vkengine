#include "world.h"

#include <fstream>
#include "node/event.h"
#include "node/physicsnode.h"

World::World() {

    this->physicsContext = new PhysicsContext();

}

World::~World() {



}

void World::addNode(std::shared_ptr<strc::Node> node) {

  this->nodes.push_back(node);
  node->worldAdd(this, node);

  std::shared_ptr<strc::PhysicsNode> pnode = std::dynamic_pointer_cast<strc::PhysicsNode>(node);

  if (pnode) {

    this->physicsContext->addObject(pnode->getPhysicsObject());
    this->entitiesByPhysicsObject[pnode->getPhysicsObject().get()] = pnode;
    
  }
  
}

void World::signalCollision(PhysicsObject *a, PhysicsObject *b, double impulse, double force) {

  std::shared_ptr<strc::Node> entityA = entitiesByPhysicsObject[a];
  std::shared_ptr<strc::Node> entityB = entitiesByPhysicsObject[b];

  entityA->eventHandler->onCollision(entityB, impulse, force);
  entityB->eventHandler->onCollision(entityA, impulse, force);
  
}

void World::simulateStep(double dt) {

  this->physicsContext->simulateStep(dt, this);

}

void World::update(double dt, double t) {

  for (std::shared_ptr<strc::Node> e : nodes) {
    e->update(dt, t);
  }

}

void World::synchronize() {

  physicsContext->synchronize();
  
  for (std::shared_ptr<strc::Node> e : nodes) {
    e->synchronize();
  }

}

void World::saveNodeState(std::string fname) {

  std::ofstream ofile(fname);

  std::shared_ptr<strc::Node> rNode = std::make_shared<strc::Node>("__root__");

  for (std::shared_ptr<strc::Node> e : nodes) {

    rNode->addChild(e);
    
  }
  
  config::save(rNode->toCompoundNode(), ofile);
  
}

std::shared_ptr<strc::Node> World::raycast(Math::Vector<3> origin, Math::Vector<3> direction, double maxLength, double & hitDist) {

  PhysicsContext::RaycastResult rcres = this->physicsContext->performRaycastClosest(origin, direction, maxLength);

  hitDist = rcres.distance;
  std::shared_ptr<strc::Node> node = nullptr;
  
  if (rcres.object) {

    node = entitiesByPhysicsObject[rcres.object];
    
  }

  return node;
  
}
