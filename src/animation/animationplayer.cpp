#include "animation/animationplayer.h"

#include <math.h>

Transform<double> AnimationPlayer::baseTransform = Transform<double>();

AnimationPlayer::AnimationPlayer() {
  current = nullptr;
}

AnimationPlayer::~AnimationPlayer() {

}

void AnimationPlayer::applyToNode(double t, strc::Node & node) {

  double x = fmod(t, current->getDuration());
  
  Transform<double> trans = this->current->getTransform(x);
  
  node.setTransform(trans);
  
}

void AnimationPlayer::addAnimation(std::string name, std::shared_ptr<Animation> anim) {

  animations[name] = anim;

  if (!current) {
    current = anim;
  }
  
}

void AnimationPlayer::plotAnimPath(std::string name, std::string nodeName, double res) {

  if (animations.find(name) == animations.end())
    return;
  
  std::string fname = nodeName.append("_").append(name).append(".csv");
  FILE * file = fopen(fname.c_str(), "w");

  std::shared_ptr<Animation> anim = this->animations[name];

  for (double d = 0; d < anim->getDuration(); d += res) {

    Transform<double> t = anim->getTransform(d);
    fprintf(file, "%lf,%lf,%lf,%lf,%lf,%lf,%lf\n", d, t.position[0], t.position[1], t.position[2], t.scale[0], t.scale[1], t.scale[2]);
    
  }

  /*const std::vector<Keyframe> & frames = anim->getKeyframes();

  for (const Keyframe & f : frames) {

    fprintf(file, "%lf,%lf,%lf,%lf\n", f.time, f.transform.position(0), f.transform.position(1), f.transform.position(2));
    
    }*/

  fclose(file);
  
}
