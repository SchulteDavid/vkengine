#include "physicscontext.h"

#include <iostream>
#include <mathutils/quaternion.h>

#include "util/debug/logger.h"
#include "util/debug/trace_exception.h"

PhysicsContext::PhysicsContext() {

    this->collisionConfig = new btDefaultCollisionConfiguration();
    this->collisionDispacher = new btCollisionDispatcher(collisionConfig);
    this->broadphaseInterface = new btDbvtBroadphase();
    this->solver = new btSequentialImpulseConstraintSolver();

    this->dynamicsWorld = new btDiscreteDynamicsWorld(collisionDispacher, broadphaseInterface, solver, collisionConfig);
    this->dynamicsWorld->setGravity(btVector3(0, 0, -9.81));
    //this->dynamicsWorld->setGravity(btVector3(0, 0, -0.9));

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

void PhysicsContext::simulateStep(double dt, CollisionHandler * handler) {

    this->simulationLock.lock();
    this->dynamicsWorld->stepSimulation(dt);

    btDispatcher * dispatcher = dynamicsWorld->getDispatcher();
    const int manifoldCount = dispatcher->getNumManifolds();

    for (int i = 0; i < manifoldCount; ++i) {

      btPersistentManifold * manifold = dispatcher->getManifoldByIndexInternal(i);

      const btRigidBody * objectA = static_cast<const btRigidBody*>(manifold->getBody0());
      const btRigidBody * objectB = static_cast<const btRigidBody*>(manifold->getBody1());

      double impulse = 0.0;
      int contacts = manifold->getNumContacts();
      for (int j = 0; j < contacts; ++j) {
	impulse += manifold->getContactPoint(j).m_appliedImpulse;
      }

      PhysicsObject * oa = (PhysicsObject *) objectA->getUserPointer();
      PhysicsObject * ob = (PhysicsObject *) objectB->getUserPointer();

      handler->signalCollision(oa, ob, impulse);

    }
    
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
    lout << "Collision Shape ok" << std::endl;
    this->collisionShapes.push_back(collisionShape);

    btTransform startTransform;
    startTransform.setIdentity();

    bool isDynamic = (obj->getMass() != 0.0);

    btVector3 localInertia(0,0,0);
    if (isDynamic) collisionShape->calculateLocalInertia(obj->getMass(), localInertia);

    Math::Quaternion<double> rotation = obj->getRotation();
    startTransform.setRotation(btQuaternion(rotation.b, rotation.c, rotation.d, rotation.a));
    startTransform.setOrigin(btVector3(obj->transform.position[0],obj->transform.position[1],obj->transform.position[2]));

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

std::shared_ptr<PhysicsObject> physutil::loadPhysicsObject(std::shared_ptr<config::NodeCompound> data, Transform<double> transform) {

  using namespace config;
  using namespace physutil;

  double mass = data->getNode<double>("mass")->getElement(0);

  std::string shapeName(data->getNode<char>("shapeName")->getRawData());

  btCollisionShape * shape = nullptr;
  
  if (shapeName == "box") {

    std::cout << "Loading BoxShape " << std::endl;
    double * s = data->getNode<double>("boxSize")->getRawData().get();
    Math::Vector<3, double> svec(s);
    svec = Math::compMultiply(svec, transform.scale);
    shape = new btBoxShape(btVector3(svec[0], svec[1], svec[2]));
    
  } else if (shapeName == "sphere") {

    double radius = data->getNode<double>("radius")->getElement(0);
    shape = new btSphereShape(radius);
    
  } else if (shapeName == "capsule") {

    double radius = data->getNode<double>("radius")->getElement(0);
    double height = data->getNode<double>("height")->getElement(0);
    
    shape = new btCapsuleShapeZ(radius, height);
    
  } else if (shapeName == "cylinder") {

    double radius = data->getNode<double>("radius")->getElement(0);
    double height = data->getNode<double>("height")->getElement(0);

    btVector3 vec(radius, radius, height/2);
    
    shape = new btCylinderShapeZ(vec);
    
  }

  if (!shape)
    throw dbg::trace_exception("Unknown collision shape");
    
  return std::make_shared<PhysicsObject>(mass, transform, shape);
  
}
