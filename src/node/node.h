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

  class EventHandler;
  
  class Node : public Resource {

  public:
    Node(std::string name);
    Node(std::string name, Transform<double> transform);
    virtual ~Node();

    void addChild(std::shared_ptr<Node> child);
    const std::vector<std::shared_ptr<Node>> & getChildren();

    void viewportAdd(Viewport * view, std::shared_ptr<Node> self);
    void worldAdd(std::shared_ptr<World> world, std::shared_ptr<Node> self);

    const Transform<double> & getTransform();
    const Transform<double> & getGlobalTransform();

    void setTransform(Transform<double> trans);
    void setTransform(Transform<double> trans, Transform<double> ptrans);

    static void registerLoaders();

    const std::string getName();

    void attachEventHandler(std::shared_ptr<EventHandler> handler);
    std::shared_ptr<EventHandler> eventHandler;

  protected:
    /// This is the transform relative to
    /// the parent node.
    Transform<double> transform;

    /// This is the absolute transform,
    /// relative to the world's origin.
    Transform<double> globalTransform;

    virtual void addToWorld(std::shared_ptr<World> world, std::shared_ptr<Node> self);
    virtual void addToViewport(Viewport * view, std::shared_ptr<Node> self);

    virtual void onTransformUpdate();

    const std::string name;

  private:
    std::vector<std::shared_ptr<Node>> children;
  };

} // namespace strc

#endif
