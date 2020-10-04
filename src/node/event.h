#ifndef EVENT_H
#define EVENT_H

#include <functional>

#include "node.h"

namespace strc {

  class EventHandler {

  public:

    void bindToParent(std::shared_ptr<Node> p);
    std::shared_ptr<Node> getParent();

    virtual void onCollision(std::shared_ptr<Node> other, double impulse, double force);

  protected:

    std::shared_ptr<Node> parent;
  
  };

  std::shared_ptr<EventHandler> constructEventHandler(std::string type);
  void registerEventHandlerType(std::string type, std::function<EventHandler *()> builder);

};

#endif
