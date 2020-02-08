#include "physicscontext.h"

#include <iostream>
#include <mathutils/quaternion.h>

PhysicsContext::PhysicsContext() {

    this->collisionConfig = new btDefaultCollisionConfiguration();
    this->collisionDispacher = new btCollisionDispatcher(collisionConfig);
    this->broadphaseInterface = new btDbvtBroadphase();
    this->solver = new btSequentialImpulseConstraintSolver();

    this->dynamicsWorld = new btDiscreteDynamicsWorld(collisionDispacher, broadphaseInterface, solver, collisionConfig);
    this->dynamicsWorld->setGravity(btVector3(0, 0, -9.81));

}

PhysicsContext::~PhysicsContext() {

    this->simulationLock.lock();
    delete this->dynamicsWorld;
    delete this->broadphaseInterface;
    delete this->collisionConfig;
    delete this->collisionDispacher;
    delete this->solver;
    this->simulationLock.unlock();


}

void PhysicsContext::simulateStep(double dt) {

    this->simulationLock.lock();
    this->dynamicsWorld->stepSimulation(dt);
    this->simulationLock.unlock();

}

void PhysicsContext::synchronize() {

    simulationLock.lock();
    for (std::shared_ptr<PhysicsObject> obj : objects) {
        obj->synchronize();
    }
    simulationLock.unlock();

}

void PhysicsContext::addObject(std::shared_ptr<PhysicsObject> obj) {

    btCollisionShape * collisionShape = obj->getCollisionShape();
    std::cout << "Collision Shape ok" << std::endl;
    this->collisionShapes.push_back(collisionShape);

    btTransform startTransform;
    startTransform.setIdentity();

    bool isDynamic = (obj->getMass() != 0.0);

    btVector3 localInertia(0,0,0);
    if (isDynamic) collisionShape->calculateLocalInertia(obj->getMass(), localInertia);

    Math::Quaternion<double> rotation = obj->getRotation();
    startTransform.setRotation(btQuaternion(rotation.b, rotation.c, rotation.d, rotation.a));
    startTransform.setOrigin(btVector3(obj->position[0],obj->position[1],obj->position[2]));

    btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(obj->getMass(),myMotionState,collisionShape,localInertia);
	btRigidBody* body = new btRigidBody(rbInfo);

	body->setLinearVelocity(btVector3(0,0,0));
	body->setAngularVelocity(btVector3(0,0,0));
	body->setFriction(1.0);

	body->setRestitution(0.1);

	body->setAngularFactor(obj->getAngularFactor());

	body->activate(true);
	//body->setLinearFactor(btVector3(1,1,1));

    dynamicsWorld->addRigidBody(body);
    obj->setRigidBody(body);

    this->objects.push_back(obj);

}
