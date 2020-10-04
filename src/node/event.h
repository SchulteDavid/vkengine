#ifndef EVENT_H
#define EVENT_H

#include "node.h"

namespace strc {

  class EventHandler {

  public:

    void bindToParent(Node * p) {
      parent = p;
    }

    virtual void onCollision(std::shared_ptr<Node> other, double impulse) {
      
    }

  private:

    Node * parent;
  
  };

};

#endif
