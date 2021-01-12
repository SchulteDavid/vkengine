#ifndef WORLD_H
#define WORLD_H

#include "entity.h"
#include "physics/physicscontext.h"

class Entity;

class World : public CollisionHandler {

public:
  World();
  virtual ~World();

  void addEntity(std::shared_ptr<Entity> entity);

  void simulateStep(double dt);
  void synchronize();

  void update(double dt, double t);

  void signalCollision(PhysicsObject * a, PhysicsObject * b, double impulse, double force);

protected:

private:

  std::vector<std::shared_ptr<Entity>> entities;
  PhysicsContext * physicsContext;

  std::unordered_map<PhysicsObject *, std::shared_ptr<Entity>> entitiesByPhysicsObject;

};

#endif // WORLD_H
