#include "lightnode.h"

#include <glm/glm.hpp>

#include "render/viewport.h"

using namespace strc;

LightNode::LightNode(std::string name, LightType type, float power, Transform<double> transform) : Node(name, transform) {
  this->type = type;
  this->power = power;
}

void LightNode::addToViewport(Viewport * view) {

  glm::vec4 pos(transform.position[0], transform.position[1], transform.position[2], (float) type);
  glm::vec4 color(power, power, power, 1.0);

  view->addLight(pos, color);

}

std::shared_ptr<NodeUploader> strc::loadLightNode(std::shared_ptr<config::NodeCompound> root, const NodeLoader::LoadingContext & context, const std::string nodeName) {

  int type = root->getNode<int>("lighttype")->getElement(0);
  float power = root->getNode<float>("power")->getElement(0);

  std::shared_ptr<strc::Node> node(new LightNode(nodeName, (LightNode::LightType) type, power, context.transform));

  return std::make_shared<NodeUploader>(node);

}
