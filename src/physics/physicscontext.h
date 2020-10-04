#ifndef PHYSICSCONTEXT_H
#define PHYSICSCONTEXT_H

#include <memory>
#include <vector>
#include <bullet/btBulletDynamicsCommon.h>
#include <mutex>
#include <configloading.h>

#include "physicsobject.h"

class CollisionHandler {

public:

  virtual void signalCollision(PhysicsObject * a, PhysicsObject * b, double impulse, double force) = 0;
  
};

class PhysicsContext {
public:
  PhysicsContext();
  virtual ~PhysicsContext();

  void simulateStep(double dt, CollisionHandler * handler);
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

namespace physutil {

  std::shared_ptr<PhysicsObject> loadPhysicsObject(std::shared_ptr<config::NodeCompound> data, Transform<double> transform);
  
};

#endif // PHYSICSCONTEXT_H
