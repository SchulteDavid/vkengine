#ifndef PHYSICSCONTEXT_H
#define PHYSICSCONTEXT_H

#include <memory>
#include <vector>
#include <bullet/btBulletDynamicsCommon.h>
#include <mutex>

#include "physicsobject.h"

class PhysicsContext {
    public:
        PhysicsContext();
        virtual ~PhysicsContext();

        void simulateStep(double dt);
        void synchronize();

        void addObject(std::shared_ptr<PhysicsObject> obj);

    protected:

    private:

        btDefaultCollisionConfiguration * collisionConfig;
        btCollisionDispatcher * collisionDispacher;
        btBroadphaseInterface * broadphaseInterface;
        btSequentialImpulseConstraintSolver * solver;

        btDiscreteDynamicsWorld * dynamicsWorld;

        btAlignedObjectArray<btCollisionShape *> collisionShapes;

        std::mutex simulationLock;

        std::vector<std::shared_ptr<PhysicsObject>> objects;

};

#endif // PHYSICSCONTEXT_H
