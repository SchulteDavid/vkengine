#include "node/event.h"

namespace strc {
  std::unordered_map<std::string, std::function<EventHandler * ()>> eventHandlerBuilders;
}

using namespace strc;

void EventHandler::bindToParent(Node *p) {
  this->parent = p;
}

void EventHandler::onCollision(std::shared_ptr<Node> other, double impulse, double force) {

}

void strc::registerEventHandlerType(std::string type, std::function<EventHandler *()> builder) {

  eventHandlerBuilders[type] = builder;
  
}

std::shared_ptr<EventHandler> strc::constructEventHandler(std::string type) {

  if (eventHandlerBuilders.find(type) == eventHandlerBuilders.end()) {
    return std::make_shared<EventHandler>();
  }

  return std::shared_ptr<EventHandler>(eventHandlerBuilders[type]());
  
}
