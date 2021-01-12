#ifndef _TRANSFORMINTERPOLATOR_H
#define _TRANSFORMINTERPOLATOR_H

#include "interpolator.h"
#include "util/transform.h"

template <typename T> class TransformInterpolator : public Interpolator<Transform<T>> {

public:
  TransformInterpolator(std::vector<double> x, std::vector<Transform<T>> y) : Interpolator<Transform<T>>(x, y) {

    std::vector<Math::Vector<3, T>> positions(y.size());
    std::vector<Math::Vector<3, T>> scales(y.size());
    rotations = std::vector<Math::Quaternion<T>>(y.size());

    for (unsigned int i = 0; i < y.size(); ++i) {
      positions[i] = y[i].position;
      scales[i] = y[i].scale;
      rotations[i] = y[i].rotation;
    }
    
    positionSpline = Math::VectorSpline<3, T>(x, positions);
    scaleSpline = Math::VectorSpline<3, T>(x, scales);
    
  }

protected:
  Transform<T> interpolate(double x) const {

    int klo, khi;

    this->findIndices(x, klo, khi);
    
    Transform<T> trans;
    trans.position = positionSpline(x);
    trans.scale = scaleSpline(x);

    double t = (x - this->xValues[klo]) / (this->xValues[khi] - this->xValues[klo]);
    trans.rotation = Math::slerp(t, rotations[klo], rotations[khi]);

    return trans;
    
  }

private:

  Math::VectorSpline<3, T> positionSpline;
  Math::VectorSpline<3, T> scaleSpline;
  std::vector<Math::Quaternion<T>> rotations;
  
};

#endif
