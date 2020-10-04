#include "entity.h"

#include "util/transform.h"

#include "node/event.h"

std::unordered_map<std::string, EntityBuilder> Entity::builders;

using namespace Math;

Entity::Entity(std::shared_ptr<strc::Node> node, std::shared_ptr<PhysicsObject> physObject) {

    this->physObject = physObject;
    this->node = node;

    this->isStatic = physObject->getMass() == 0;

}

Entity::~Entity()
{
    //dtor
}

void Entity::synchronize() {

    if (isStatic) return;

    /*physObject->synchronize();

    if (!renderElement) return;

    Transform<float> trans;
    float tmp[3] = {(float) physObject->position[0], (float) physObject->position[1], (float) physObject->position[2]};
    trans.position = Math::Vector<3, float>(tmp);
    trans.rotation = Math::Quaternion<float>(physObject->rotation.a, physObject->rotation.b, physObject->rotation.c, physObject->rotation.d);
    trans.scale = Math::Vector<3, float>({1,1,1});

    this->renderElement->updateInstance(renderInstance, trans);*/

    Transform<double> trans = physObject->getTransform();
    //std::cout << "Setting transform for entity " << trans << std::endl;
    this->node->setTransform(trans);

}

std::shared_ptr<Entity> Entity::buildEntityFromType(std::string type, std::shared_ptr<strc::Node> node, std::shared_ptr<PhysicsObject> physObject) {

    if (builders.find(type) == builders.end()) {
        throw dbg::trace_exception(std::string("No builter registered for entity type '").append(type).append("'"));
    }

    return builders[type](node, physObject);

}

void Entity::registerEntityType(std::string type, EntityBuilder builder) {

    builders[type] = builder;

}

void Entity::registerDefaultEntityTypes() {

  registerEntityType("Entity", [] (std::shared_ptr<strc::Node> node, std::shared_ptr<PhysicsObject> physObject) -> std::shared_ptr<Entity> {
        return std::shared_ptr<Entity>(new Entity(node, physObject));
    });

}

void Entity::onCollision(std::shared_ptr<Entity> entity, double impulse, double force) {
  this->node->eventHandler->onCollision(entity->node, impulse, force);
}

void Entity::onUpdate(const double dt) {

}

std::shared_ptr<PhysicsObject> Entity::getPhysicsObject() {
    return this->physObject;
}


void Entity::applyImpulse(Vector<3> impulse) {

    lout << "EntityImpulse " << impulse << std::endl;
    this->physObject->applyImpulse(impulse);

}

void Entity::applyForce(Vector<3> force, Vector<3> pos) {
    this->physObject->applyForce(force, pos);
}

double Entity::getMass() {
    return physObject->getMass();
}

Vector<3> Entity::getPosition() {
    return physObject->getPosition();
}
