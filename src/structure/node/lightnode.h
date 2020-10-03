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

    void addToViewport(Viewport * view) override;

  private:

    LightType type;
    float power;


  };

  std::shared_ptr<NodeUploader> loadLightNode(std::shared_ptr<config::NodeCompound> root, const NodeLoader::LoadingContext & context, const std::string nodeName);

};

#endif
