#ifndef STRUCTURE_NODE_H
#define STRUCTURE_NODE_H

#include <memory>

#include "util/transform.h"
#include "resources/resourceloader.h"

#include <unordered_map>
#include <functional>
#include <configloading.h>

class Viewport;
class World;

namespace strc {

  class Node : public Resource {
    
  public:
    Node();
    Node(Transform<double> transform);
    virtual ~Node();
    
    void addChild(std::shared_ptr<Node> child);
    const std::vector<std::shared_ptr<Node>> & getChildren();

    void viewportAdd(Viewport * view);

    void transformSet(Transform<double> transform, Transform<double> parenTransform);

    const Transform<double> & getTransform();
    const Transform<double> & getGlobalTransform();

    static void registerLoaders();
    
  protected:
    /// This is the transform relative to
    /// the parent node.
    Transform<double> transform;

    /// This is the absolute transform,
    /// relative to the world's origin.
    Transform<double> globalTransform;

    virtual void addToWorld(std::shared_ptr<World> world);
    virtual void addToViewport(Viewport * view);
    virtual void setTransform(Transform<double> trans);
    virtual void setParentTransform(Transform<double> trans);
    
  private:
    std::vector<std::shared_ptr<Node>> children;
  };
  
} // namespace strc

#endif
