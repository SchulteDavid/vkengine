#include "physicsobject.h"

using namespace Math;

PhysicsObject::PhysicsObject(double mass, Math::Vector<3,double> pos, Math::Quaternion<double> rot) : PhysicsObject(mass, Transform<double>(pos, rot)) {

}

PhysicsObject::PhysicsObject(double mass, Math::Vector<3,double> pos, Math::Quaternion<double> rot, btCollisionShape * collisionShape)
  : PhysicsObject(mass, Transform<double>(pos, rot), collisionShape) {

}

PhysicsObject::PhysicsObject(double mass, Transform<double> transform) : PhysicsObject(mass, transform, new btBoxShape(btVector3(1,1,1))) {

}

PhysicsObject::PhysicsObject(double mass, Transform<double> transform, btCollisionShape * collisionShape) {

  this->mass = mass;
  this->transform = transform;
  this->collisionShape = collisionShape;
  this->angularFactor = 1.0;
  this->rigidBody = nullptr;
  
}

PhysicsObject::~PhysicsObject()
{
    //dtor
}

double PhysicsObject::getMass() {
    return mass;
}

btCollisionShape * PhysicsObject::getCollisionShape() {
    return collisionShape;
}

Math::Quaternion<double> PhysicsObject::getRotation() {
    return transform.rotation;
}

Math::Vector<3, double> PhysicsObject::getPosition() {
    return transform.position;
}

double PhysicsObject::getAngularFactor() {
    return this->angularFactor;
}

void PhysicsObject::setRigidBody(btRigidBody * body) {

    this->rigidBody = body;
    this->rigidBody->setUserPointer(this);

}

Transform<double> PhysicsObject::getTransform() {

  return transform;
  
}

void PhysicsObject::synchronize() {

    if (!this->rigidBody || !this->rigidBody->getMotionState())
        return;

    btTransform trans = rigidBody->getCenterOfMassTransform();
    double tmp[3] = {trans.getOrigin().getX(), trans.getOrigin().getY(), trans.getOrigin().getZ()};
    transform.position = Vector<3,double>(tmp);
    transform.rotation = Quaternion<double>(trans.getRotation().getW(), trans.getRotation().getX(), trans.getRotation().getY(), trans.getRotation().getZ());

}

void PhysicsObject::setTransform(Transform<double> trans) {
  transform = trans;

  if (this->rigidBody && this->rigidBody->getMotionState()) {
    btTransform t;
    btQuaternion r(trans.rotation.a, trans.rotation.b, trans.rotation.c, trans.rotation.d);
    t.setOrigin(btVector3(trans.position[0], trans.position[1], trans.position[2]));
    t.setRotation(r);
    rigidBody->setCenterOfMassTransform(t);
  }
  
}

void PhysicsObject::applyForce(const Vector<3> & force, const Vector<3> & pos) {

    this->rigidBody->activate(true);
    this->rigidBody->applyForce(btVector3(force(0), force(1), force(2)), btVector3(pos(0), pos(1), pos(2)));

}

void PhysicsObject::applyImpulse(const Vector<3> & impulse, const Vector<3> & position) {

    this->rigidBody->activate(true);
    this->rigidBody->applyImpulse(btVector3(impulse(0), impulse(1), impulse(2)),
				  btVector3(position(0), position(1), position(2)));

}

void PhysicsObject::setAngularFactor(double f) {
    this->angularFactor = f;
}

void PhysicsObject::performRaycast() {
  
}

void PhysicsObject::setShapeType(std::string shapeType) {
  this->shapeType = shapeType;
}

void PhysicsObject::setShapeData(std::any shapeData) {
  this->shapeData = shapeData;
}

std::string PhysicsObject::getShapeType() {
  return shapeType;
}

std::any PhysicsObject::getShapeData() {
  return shapeData;
}
