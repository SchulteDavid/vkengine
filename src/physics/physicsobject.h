#ifndef PHYSICSOBJECT_H
#define PHYSICSOBJECT_H

#include <bullet/btBulletDynamicsCommon.h>

#include <mathutils/quaternion.h>
#include <mathutils/vector.h>

#include "util/transform.h"

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

  void setRigidBody(btRigidBody * body);

  void applyImpulse(Math::Vector<3, double> impulse);
  void applyForce(Math::Vector<3, double> force, Math::Vector<3, double> pos);

  void synchronize();

protected:

private:

  double mass;
  double angularFactor;
  btCollisionShape * collisionShape;
  btRigidBody * rigidBody;


};

#endif // PHYSICSOBJECT_H
