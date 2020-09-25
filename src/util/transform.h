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

#endif
