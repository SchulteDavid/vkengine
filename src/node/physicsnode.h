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

  protected:

    void addToWorld(std::shared_ptr<World> world, std::shared_ptr<Node> self) override;

  private:
    std::shared_ptr<PhysicsObject> physObject;

  };

  std::shared_ptr<NodeUploader> loadPhysicsNode(std::shared_ptr<config::NodeCompound> root, const NodeLoader::LoadingContext & context, const std::string nodeName);

};

#endif
