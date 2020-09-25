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

    static void registerLoaders();
    
  protected:
    Transform<double> transform;

    virtual void addToWorld(std::shared_ptr<World> world);
    virtual void addToViewport(Viewport * view);
    
  private:
    std::vector<std::shared_ptr<Node>> children;
  };
  
} // namespace strc

#endif
