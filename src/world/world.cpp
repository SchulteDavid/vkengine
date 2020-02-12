#include "world.h"

World::World() {

    this->physicsContext = new PhysicsContext();

}

World::~World() {



}

void World::addEntity(std::shared_ptr<Entity> entity) {

    this->entities.push_back(entity);
    this->physicsContext->addObject(entity->getPhysicsObject());

}

void World::simulateStep(double dt) {

    this->physicsContext->simulateStep(dt);

    /*for (std::shared_ptr<Entity> e : entities) {
        std::cout << "Updating " << e << std::endl;
        e->onUpdate(dt);
    }*/

}

void World::update(double dt) {

    for (std::shared_ptr<Entity> e : entities) {
        e->onUpdate(dt);
    }

}

void World::synchronize() {

    for (std::shared_ptr<Entity> e : entities) {
        e->synchronize();
    }

}
