#include "physicsobject.h"

using namespace Math;

PhysicsObject::PhysicsObject(double mass, Math::Vector<3,double> pos, Math::Quaternion<double> rot) {

    this->mass = mass;
    this->collisionShape = new btBoxShape(btVector3(1,1,1));
    this->angularFactor = 1.0;
    this->position = pos;
    this->rotation = rot;
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
    return rotation;
}

Math::Vector<3, double> PhysicsObject::getPosition() {
    return position;
}

double PhysicsObject::getAngularFactor() {
    return this->angularFactor;
}

void PhysicsObject::setRigidBody(btRigidBody * body) {

    this->rigidBody = body;

}

void PhysicsObject::synchronize() {

    if (!this->rigidBody || !this->rigidBody->getMotionState())
        return;

    btTransform trans = rigidBody->getCenterOfMassTransform();
    double tmp[3] = {trans.getOrigin().getX(), trans.getOrigin().getY(), trans.getOrigin().getZ()};
    this->position = Vector<3,double>(tmp);
    this->rotation = Quaternion<double>(trans.getRotation().getW(), trans.getRotation().getX(), trans.getRotation().getY(), trans.getRotation().getZ());

}
