#include "lightnode.h"

#include <glm/glm.hpp>

#include "render/viewport.h"

using namespace strc;

LightNode::LightNode(std::string name, LightType type, float power, Transform<double> transform) : Node(name, transform) {
  this->type = type;
  this->power = power;
  this->view = nullptr;
}

void LightNode::addToViewport(Viewport * view, std::shared_ptr<Node> self) {

  Transform<double> trans = getGlobalTransform();
  glm::vec4 pos(trans.position[0], trans.position[1], trans.position[2], (float) type);
  glm::vec4 color(power, power, power, 1.0);

  this->light = view->addLight(pos, color);

  this->view = view;
  

}

void LightNode::onTransformUpdate() {
  
  if (!view) return;

  Transform<double> trans = getGlobalTransform();
  glm::vec4 pos(trans.position[0], trans.position[1], trans.position[2], (float) type);
  glm::vec4 color(power, power, power, 1.0);
  
  this->view->updateLight(light, pos, color);
  
}

std::shared_ptr<NodeUploader> strc::loadLightNode(std::shared_ptr<config::NodeCompound> root, const NodeLoader::LoadingContext & context, const std::string nodeName) {

  int type = root->getNode<int>("lighttype")->getElement(0);
  float power = root->getNode<float>("power")->getElement(0);

  std::shared_ptr<strc::Node> node(new LightNode(nodeName, (LightNode::LightType) type, power, context.transform));

  return std::make_shared<NodeUploader>(node);

}
