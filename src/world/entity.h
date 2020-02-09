#ifndef ENTITY_H
#define ENTITY_H

#include <functional>

#include "render/renderelement.h"
#include "physics/physicsobject.h"

class Entity;

typedef std::function<Entity *(std::shared_ptr<RenderElement> renderElement, RenderElement::Instance instance, std::shared_ptr<PhysicsObject> physObject)> EntityBuilder;

class Entity
{
    public:
        Entity(std::shared_ptr<RenderElement> renderElement, RenderElement::Instance instance, std::shared_ptr<PhysicsObject> physObject);
        virtual ~Entity();

        virtual void onCollision(Entity * entity);
        virtual void synchronize();

        std::shared_ptr<PhysicsObject> getPhysicsObject();

        static void registerEntityType(std::string type, EntityBuilder builder);
        static void registerDefaultEntityTypes();
        static Entity * buildEntityFromType(std::string type, std::shared_ptr<RenderElement> renderElement, RenderElement::Instance instance, std::shared_ptr<PhysicsObject> physObject);

    protected:

    private:

        bool isStatic;

        std::shared_ptr<RenderElement> renderElement;
        RenderElement::Instance renderInstance;
        std::shared_ptr<PhysicsObject> physObject;

        static std::unordered_map<std::string, EntityBuilder> builders;


};

#endif // ENTITY_H
