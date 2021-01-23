#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <mathutils/matrix.h>
#include <mathutils/quaternion.h>

template <typename T = float> struct Transform {

  Transform(){
    position = Math::Vector<3, T>({0,0,0});
    rotation = Math::Quaternion<T>(1,0,0,0);
    scale = Math::Vector<3, T>({1,1,1});
  };

  Transform(Math::Vector<3, T> pos, Math::Quaternion<T> rot) {
    position = pos;
    rotation = rot;
    scale = Math::Vector<3, T>({1,1,1});
  }

  Math::Vector<3, T> position;
  Math::Quaternion<T> rotation;
  Math::Vector<3, T> scale;
  
};

template <typename R, typename T> Transform<T> convertTransform(Transform<R> t) {

  Transform<T> res;
  res.position = Math::Vector<3, T>({(T) t.position[0], (T) t.position[1], (T) t.position[2]});
  res.rotation = Math::Quaternion<T>((T) t.rotation.a, (T) t.rotation.b, (T) t.rotation.c, (T) t.rotation.d);
  res.scale = Math::Vector<3, T>({(T) t.scale[0], (T) t.scale[1], (T) t.scale[2]});

  return res;
}

template <typename T=float>  Math::Matrix<4, 4, T> getTransformationMatrix(Transform<T> & trans) {

  Math::Matrix<4, 4, T> prMat = trans.rotation.toModelMatrix(trans.position);
  Math::Matrix<4, 4, T> sMat({
      trans.scale[0], 0, 0, 0,
	0, trans.scale[1], 0, 0,
	0, 0, trans.scale[2], 0,
	0, 0, 0, 1
    });

  return prMat * sMat;
  
}

template <typename T> std::ostream & operator<<(std::ostream & stream, Transform<T> & trans) {

  return (stream << "position:" << trans.position << " scale:" << trans.scale << " rotation:" << trans.rotation);
  
}

template <typename T> inline Transform<T> operator*(const Transform<T> & t1, const Transform<T> & t2) {

  Transform<T> res;
  res.position = t1.position + t1.rotation.toRotationMatrix() * Math::compMultiply(t1.scale, t2.position);
  res.rotation = t1.rotation * t2.rotation;
  res.scale = Math::compMultiply(t1.scale, t2.scale);

  return res;
  
}

/// Use this for transforms in the same space, not relative to one another
template <typename T> Transform<T> combineTransforms(const Transform<T> & t1, const Transform<T> & t2) {

  Transform<T> res;
  res.position = t1.position + t1.rotation.toRotationMatrix() * Math::compMultiply(t1.scale, t2.position);
  res.rotation = t1.rotation * t2.rotation;
  res.scale = Math::compMultiply(t1.scale, t2.scale);

  return res;
  
}

template <typename T> Transform<T> inverseTransform(const Transform<T> trans) {
  Transform<T> res;
  res.position = ((T) -1) * trans.position;
  res.rotation = trans.rotation.conjugate();
  res.scale = trans.scale;
  return res;
}

template <typename T> Math::Quaternion<T> rotationBetweenVectors(Math::Vector<3, T> start, Math::Vector<3, T> dest) {

  start.normalize();
  dest.normalize();

  T cosTheta = start * dest;
  Math::Vector<3, T> rotationAxis;

  if (cosTheta < (-1 + 0.001)) {

    rotationAxis = Math::cross(Math::Vector<3, T>(0,1,0), start);
    if (rotationAxis.length() < 0.001){
      rotationAxis = Math::cross(Math::Vector<3, T>(1,0,0), start);
    }

    rotationAxis.normalize();
    return Math::Quaternion<T>::fromAxisAngle(rotationAxis, M_PI);
    
  }

  rotationAxis = Math::cross(start, dest);

  T s = sqrt(2 * (1 + cosTheta));

  return Math::Quaternion<T>(s/2, rotationAxis[0] / s, rotationAxis[1] / s, rotationAxis[2] / s);
  
}

#endif
