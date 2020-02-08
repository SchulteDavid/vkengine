#ifndef ENTITY_H
#define ENTITY_H

#include "render/renderelement.h"
#include "physics/physicsobject.h"

class Entity
{
    public:
        Entity(std::shared_ptr<RenderElement> renderElement, RenderElement::Instance instance, std::shared_ptr<PhysicsObject> physObject);
        virtual ~Entity();

        void synchronize();

    protected:

    private:

        std::shared_ptr<RenderElement> renderElement;
        RenderElement::Instance renderInstance;
        std::shared_ptr<PhysicsObject> physObject;


};

#endif // ENTITY_H
