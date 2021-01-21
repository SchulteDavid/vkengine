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

  struct RaycastResult {
    PhysicsObject * object;
    double distance;
  };
  
  PhysicsContext();
  virtual ~PhysicsContext();

  void simulateStep(double dt, CollisionHandler * handler);
  void synchronize();

  void addObject(std::shared_ptr<PhysicsObject> obj);

  /// Returns closest element on raycast.
  RaycastResult performRaycastClosest(Math::Vector<3> origin, Math::Vector<3> direction, double maxLength);

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

#include "resources/resourceloader.h"

namespace physutil {

  std::shared_ptr<PhysicsObject> loadPhysicsObject(std::shared_ptr<config::NodeCompound> data, Transform<double> transform, const std::unordered_map<std::string, LoadingResource> & attachedResources);
  
};

#endif // PHYSICSCONTEXT_H
