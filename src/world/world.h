#ifndef WORLD_H
#define WORLD_H

#include "entity.h"
#include "physics/physicscontext.h"

class Entity;

class World {

    public:
        World();
        virtual ~World();

        void addEntity(std::shared_ptr<Entity> entity);

        void simulateStep(double dt);
        void synchronize();

        void update(double dt);

    protected:

    private:

        std::vector<std::shared_ptr<Entity>> entities;
        PhysicsContext * physicsContext;

};

#endif // WORLD_H
