#include "animation.h"

#include "animation/interpolation/transforminterpolator.h"

Animation::Animation() {

  interpolator = nullptr;
  
}

Animation::Animation(std::vector<Keyframe> & keyframes) {
  std::vector<double> x(keyframes.size());
  std::vector<Transform<double>> y(keyframes.size());

  for (unsigned int i = 0; i < keyframes.size(); ++i) {
    x[i] = keyframes[i].time;
    y[i] = keyframes[i].transform;
  }
  
  interpolator = new TransformInterpolator<double>(x, y);

  frames = keyframes;

}

Animation::~Animation() {

  if (interpolator)
    delete interpolator;
  
}

double Animation::getDuration() {
  return frames[frames.size()-1].time;
}

Transform<double> Animation::getTransform(double t) {
  return (*interpolator)(t);
}

const std::vector<Keyframe> & Animation::getKeyframes() {
  return this->frames;
}
