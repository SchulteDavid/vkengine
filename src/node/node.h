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
class Entity;
class AnimationPlayer;
class Animation;

namespace strc {

  class EventHandler;
  
  class Node : public Resource {

  public:
    Node(std::string name);
    Node(std::string name, Transform<double> transform);
    virtual ~Node();

    void addChild(std::shared_ptr<Node> child);
    std::shared_ptr<Node> getChild(std::string name);

    void viewportAdd(Viewport * view, std::shared_ptr<Node> self);
    void worldAdd(std::shared_ptr<World> world, std::shared_ptr<Node> self);

    const Transform<double> & getTransform();
    const Transform<double> & getGlobalTransform();
    const Transform<double> & getParentTransform();

    void setTransform(Transform<double> trans);
    void setTransform(Transform<double> trans, Transform<double> ptrans);

    static void registerLoaders();

    const std::string getName();

    void attachEventHandler(std::shared_ptr<EventHandler> handler, std::shared_ptr<Node> self);
    std::shared_ptr<EventHandler> eventHandler;

    void attachResource(std::string name, std::shared_ptr<Resource> resource);

    std::shared_ptr<Resource> getAttachedResource(std::string name);
    
    template <typename T> std::shared_ptr<T> getResource(std::string name) {
      if (attachedResources.find(name) == attachedResources.end()) {
	throw dbg::trace_exception(std::string("No such attached Resource ").append(name));
      }
      std::shared_ptr<T> res = std::dynamic_pointer_cast<T>(attachedResources[name]);
      if (!res) {
	throw dbg::trace_exception(std::string("Wrong resource type for ").append(name));
      }
      return res;
    }

    void update(const double dt, const double t);

    std::shared_ptr<Node> createDuplicate(std::string newName);

    void addAnimation(std::string name, std::shared_ptr<Animation> animation);
    
  protected:
    friend Entity;
    /// This is the transform relative to
    /// the parent node.
    Transform<double> transform;

    /// This is the absolute transform,
    /// relative to the world's origin.
    Transform<double> globalTransform;

    /// This is the absolute transform of the nodes parent
    Transform<double> parentTransform;

    virtual void addToWorld(std::shared_ptr<World> world, std::shared_ptr<Node> self);
    virtual void addToViewport(Viewport * view, std::shared_ptr<Node> self);
    virtual void onUpdate(const double dt, const double t);

    virtual void onTransformUpdate();

    virtual std::shared_ptr<Node> duplicate(std::string name);

    const std::string name;

    std::shared_ptr<AnimationPlayer> animationPlayer;

  private:
    std::unordered_map<std::string, std::shared_ptr<Node>> children;
    std::unordered_map<std::string, std::shared_ptr<Resource>> attachedResources;
    
  };

} // namespace strc

#endif
