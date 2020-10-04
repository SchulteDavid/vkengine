#ifndef EVENT_H
#define EVENT_H

#include <functional>

#include "node.h"

namespace strc {

  class EventHandler {

  public:

    void bindToParent(Node * p);

    virtual void onCollision(std::shared_ptr<Node> other, double impulse);

  private:

    Node * parent;
  
  };

  std::shared_ptr<EventHandler> constructEventHandler(std::string type);
  void registerEventHandlerType(std::string type, std::function<EventHandler *()> builder);

};

#endif
