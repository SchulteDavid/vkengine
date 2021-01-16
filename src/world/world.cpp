#include "world.h"

#include <fstream>

World::World() {

    this->physicsContext = new PhysicsContext();

}

World::~World() {



}

void World::addEntity(std::shared_ptr<Entity> entity) {

    this->entities.push_back(entity);
    this->physicsContext->addObject(entity->getPhysicsObject());

    this->entitiesByPhysicsObject[entity->getPhysicsObject().get()] = entity;

}

void World::signalCollision(PhysicsObject *a, PhysicsObject *b, double impulse, double force) {

  std::shared_ptr<Entity> entityA = entitiesByPhysicsObject[a];
  std::shared_ptr<Entity> entityB = entitiesByPhysicsObject[b];

  entityA->onCollision(entityB, impulse, force);
  entityB->onCollision(entityA, impulse, force);
  
}

void World::simulateStep(double dt) {

  this->physicsContext->simulateStep(dt, this);

}

void World::update(double dt, double t) {

  for (std::shared_ptr<Entity> e : entities) {
    e->onUpdate(dt, t);
  }

}

void World::synchronize() {

  physicsContext->synchronize();
  
  for (std::shared_ptr<Entity> e : entities) {
    e->synchronize();
  }

}

void World::saveNodeState(std::string fname) {

  std::ofstream ofile(fname);

  std::shared_ptr<strc::Node> rNode = std::make_shared<strc::Node>("__root__");

  for (std::shared_ptr<Entity> e : entities) {

    rNode->addChild(e->getNode());
    
  }
  
  config::save(rNode->toCompoundNode(), ofile);
  
}
