#include "entity.h"

Entity::Entity(std::shared_ptr<RenderElement> renderElement, RenderElement::Instance instance, std::shared_ptr<PhysicsObject> physObject) {

    this->renderInstance = instance;
    this->physObject = physObject;
    this->renderElement = renderElement;

}

Entity::~Entity()
{
    //dtor
}

void Entity::synchronize() {

    physObject->synchronize();

    RenderElement::Transform trans;
    float tmp[4] = {physObject->position[0], physObject->position[1], physObject->position[2], 0.0};
    trans.position = Math::Vector<4, float>(tmp);
    trans.qRot = Math::Quaternion<float>(physObject->rotation.a, physObject->rotation.b, physObject->rotation.c, physObject->rotation.d);
    trans.scale = 1;

    this->renderElement->updateInstance(renderInstance, trans);

}
