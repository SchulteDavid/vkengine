#include "node/event.h"

namespace strc {
  std::unordered_map<std::string, std::function<EventHandler * ()>> eventHandlerBuilders;
}

using namespace strc;

void EventHandler::bindToParent(std::shared_ptr<Node> p) {
  this->parent = p;
}

void EventHandler::onCollision(std::shared_ptr<Node> other, double impulse, double force) {

}

void strc::registerEventHandlerType(std::string type, std::function<EventHandler *()> builder) {

  eventHandlerBuilders[type] = builder;
  
}

std::shared_ptr<Node> strc::EventHandler::getParent() {
  return parent;
}

std::shared_ptr<EventHandler> strc::constructEventHandler(std::string type) {

  std::cout << "Creating event handler of type " << type << std::endl;
  
  if (eventHandlerBuilders.find(type) == eventHandlerBuilders.end()) {
    return std::make_shared<EventHandler>();
  }

  return std::shared_ptr<EventHandler>(eventHandlerBuilders[type]());
  
}
