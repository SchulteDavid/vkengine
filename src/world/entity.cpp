#include "entity.h"

std::unordered_map<std::string, EntityBuilder> Entity::builders;

using namespace Math;

Entity::Entity(std::shared_ptr<RenderElement> renderElement, RenderElement::Instance instance, std::shared_ptr<PhysicsObject> physObject) {

    this->renderInstance = instance;
    this->physObject = physObject;
    this->renderElement = renderElement;

    this->isStatic = physObject->getMass() == 0;

}

Entity::~Entity()
{
    //dtor
}

void Entity::synchronize() {

    if (isStatic) return;

    physObject->synchronize();

    RenderElement::Transform trans;
    float tmp[4] = {physObject->position[0], physObject->position[1], physObject->position[2], 0.0};
    trans.position = Math::Vector<4, float>(tmp);
    trans.qRot = Math::Quaternion<float>(physObject->rotation.a, physObject->rotation.b, physObject->rotation.c, physObject->rotation.d);
    trans.scale = 1;

    this->renderElement->updateInstance(renderInstance, trans);

}

std::shared_ptr<Entity> Entity::buildEntityFromType(std::string type, std::shared_ptr<RenderElement> renderElement, RenderElement::Instance instance, std::shared_ptr<PhysicsObject> physObject) {

    if (builders.find(type) == builders.end()) {
        throw dbg::trace_exception(std::string("No builter registered for entity type '").append(type).append("'"));
    }

    return builders[type](renderElement, instance, physObject);

}

void Entity::registerEntityType(std::string type, EntityBuilder builder) {

    builders[type] = builder;

}

void Entity::registerDefaultEntityTypes() {

    registerEntityType("Entity", [] (std::shared_ptr<RenderElement> renderElement, RenderElement::Instance instance, std::shared_ptr<PhysicsObject> physObject) -> std::shared_ptr<Entity> {
        return std::shared_ptr<Entity>(new Entity(renderElement, instance, physObject));
    });

}

void Entity::onCollision(Entity * entity) {

}

void Entity::onUpdate(const double dt) {

}

std::shared_ptr<PhysicsObject> Entity::getPhysicsObject() {
    return this->physObject;
}


void Entity::applyImpulse(Vector<3> impulse) {

    std::cout << "EntityImpulse " << impulse << std::endl;
    this->physObject->applyImpulse(impulse);

}

void Entity::applyForce(Vector<3> force, Vector<3> pos) {
    this->physObject->applyForce(force, pos);
}

double Entity::getMass() {
    return physObject->getMass();
}
