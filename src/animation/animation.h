#ifndef ANIMATION_H
#define ANIMATION_H

#include <mathutils/quaternion.h>
#include <mathutils/vector.h>

#include "util/transform.h"
#include "animation/interpolation/interpolator.h"

struct Keyframe {

  double time;
  Transform<double> transform;

};

class Animation {

public:
  Animation(std::vector<Keyframe> & keyframes);
  Animation();
  virtual ~Animation();

  double getDuration();
  Transform<double> getTransform(double t);

  const std::vector<Keyframe> & getKeyframes();
  
protected:
  
private:
  
  std::vector<Keyframe> frames;
  Interpolator<Transform<double>> * interpolator;

};

#endif // ANIMATION_H
