#ifndef _ANIMATIONPLAYER_H
#define _ANIMATIONPLAYER_H

#include <unordered_map>
#include <memory>

#include "animation/animation.h"
#include "node/node.h"


class AnimationPlayer {

public:
  AnimationPlayer();
  virtual ~AnimationPlayer();

  void applyToNode(double t, strc::Node & node);
  void addAnimation(std::string name, std::shared_ptr<Animation> anim);

  void plotAnimPath(std::string name, std::string nodeName, double res = 0.01);

private:

  static Transform<double> baseTransform;
  
  std::shared_ptr<Animation> current;
  std::unordered_map<std::string, std::shared_ptr<Animation>> animations;
  
};

#endif
