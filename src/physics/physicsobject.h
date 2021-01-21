#ifndef PHYSICSOBJECT_H
#define PHYSICSOBJECT_H

#include <bullet/btBulletDynamicsCommon.h>

#include <mathutils/quaternion.h>
#include <mathutils/vector.h>

#include "util/transform.h"

#include <any>

class PhysicsObject {

public:

  Transform<double> transform;

  PhysicsObject(double mass, Math::Vector<3,double> pos, Math::Quaternion<double> rot);
  PhysicsObject(double mass, Math::Vector<3,double> pos, Math::Quaternion<double> rot, btCollisionShape * collisionShape);
  PhysicsObject(double mass, Transform<double> transform);
  PhysicsObject(double mass, Transform<double> transform, btCollisionShape * collisionShape);
  virtual ~PhysicsObject();

  double getMass();
  double getAngularFactor();
  void setAngularFactor(double f);
  btCollisionShape * getCollisionShape();

  Math::Quaternion<double> getRotation();
  Math::Vector<3, double> getPosition();

  Transform<double> getTransform();

  void setTransform(Transform<double> trans);
  
  void setRigidBody(btRigidBody * body);

  void applyImpulse(const Math::Vector<3, double> & impulse, const Math::Vector<3> & position);
  void applyForce(const Math::Vector<3, double> & force, const Math::Vector<3, double> & pos);

  void synchronize();

  void performRaycast();

  void setShapeType(std::string shapeType);
  void setShapeData(std::any shapeData);

  std::string getShapeType();
  std::any getShapeData();

protected:

private:

  double mass;
  double angularFactor;
  btCollisionShape * collisionShape;
  btRigidBody * rigidBody;

  std::string shapeType;
  std::any shapeData;


};

#endif // PHYSICSOBJECT_H
