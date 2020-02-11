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

PhysicsObject::PhysicsObject(double mass, Math::Vector<3,double> pos, Math::Quaternion<double> rot, btCollisionShape * collisionShape) {

    this->mass = mass;
    this->collisionShape = collisionShape;
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

void PhysicsObject::applyForce(Vector<3> force, Vector<3> pos) {

    this->rigidBody->activate(true);
    this->rigidBody->applyForce(btVector3(force[0], force[1], force[2]), btVector3(pos[0], pos[1], pos[2]));

}

void PhysicsObject::applyImpulse(Vector<3> impulse) {

    this->rigidBody->activate(true);
    this->rigidBody->applyCentralImpulse(btVector3(impulse[0], impulse[1], impulse[2]));

}
