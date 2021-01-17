#ifndef WORLD_H
#define WORLD_H

#include "node/node.h"
#include "physics/physicscontext.h"

class World : public CollisionHandler {

public:
  World();
  virtual ~World();

  void addNode(std::shared_ptr<strc::Node> node);

  void simulateStep(double dt);
  void synchronize();

  void update(double dt, double t);

  void signalCollision(PhysicsObject * a, PhysicsObject * b, double impulse, double force);

  void saveNodeState(std::string fname);

  std::shared_ptr<strc::Node> raycast(Math::Vector<3> origin, Math::Vector<3> direction, double maxLength, double & hitDist);
  
protected:

private:

  std::vector<std::shared_ptr<strc::Node>> nodes;
  PhysicsContext * physicsContext;

  std::unordered_map<PhysicsObject *, std::shared_ptr<strc::Node>> entitiesByPhysicsObject;

};

#endif // WORLD_H
