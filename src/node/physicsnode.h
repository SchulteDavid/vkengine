#ifndef PHYSICS_NODE_H
#define PHYSICS_NODE_H

#include "node.h"
#include "physics/physicsobject.h"
#include "node/nodeloader.h"

namespace strc {

  class PhysicsNode : public Node {

  public:
    PhysicsNode(std::string name, Transform<double> transform, std::shared_ptr<PhysicsObject> physObject);
    virtual ~PhysicsNode();

    void applyImpulse(Math::Vector<3> impulse, Math::Vector<3> position);
    void applyForce(Math::Vector<3> forcem, Math::Vector<3> position);
    double getMass();

    std::shared_ptr<PhysicsObject> getPhysicsObject();

    void synchronize() override;

  protected:

    void addToWorld(World * world, std::shared_ptr<Node> self) override;
    std::shared_ptr<Node> duplicate(std::string name) override;
    void onTransformUpdate() override;
    void onUpdate(const double dt, const double t) override;

  private:
    std::shared_ptr<PhysicsObject> physObject;
    World * world;

    bool isInSimulation;

  };

  std::shared_ptr<NodeUploader> loadPhysicsNode(std::shared_ptr<config::NodeCompound> root, const NodeLoader::LoadingContext & context, const std::string nodeName);

};

#endif
