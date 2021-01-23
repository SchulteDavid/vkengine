#ifndef LIGHT_NODE_H
#define LIGHT_NODE_H

#include "node.h"
#include "nodeloader.h"

namespace strc {

  class LightNode : public Node {

  public:

    enum LightType {
		    LIGHT_NONE = 0,
		    LIGHT_POINT,
		    LIGHT_SUN,
    };

    LightNode(std::string name, LightType type, float power, Transform<double> transform);
    LightNode(std::string name, LightType type, float power, Math::Vector<3, float> color, Transform<double> transform);

    void saveNode(std::shared_ptr<config::NodeCompound> comp) override;
    
  protected:
    void addToViewport(Viewport * view, std::shared_ptr<Node> self) override;
    void onTransformUpdate() override;

    std::shared_ptr<Node> duplicate(std::string name) override;

  private:

    Viewport * view;
    uint32_t light;
    
    LightType type;
    float power;
    Math::Vector<3, float> color;


  };

  std::shared_ptr<NodeUploader> loadLightNode(std::shared_ptr<config::NodeCompound> root, const NodeLoader::LoadingContext & context, const std::string nodeName);

};

#endif
