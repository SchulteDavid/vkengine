#include "physicscontext.h"

#include <iostream>
#include <mathutils/quaternion.h>

#include "util/debug/logger.h"
#include "util/debug/trace_exception.h"

#include "util/mesh.h"

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

    if (!contacts) continue;
    
    for (int j = 0; j < contacts; ++j) {
      impulse += manifold->getContactPoint(j).m_appliedImpulse;
    }

    double force = impulse / dt;

    PhysicsObject * oa = (PhysicsObject *) objectA->getUserPointer();
    PhysicsObject * ob = (PhysicsObject *) objectB->getUserPointer();

    handler->signalCollision(oa, ob, impulse, force);

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

namespace physutil {

  void transferMeshDataToBullet(std::shared_ptr<Mesh> mesh, btTriangleMesh * btMesh) {

    std::vector<uint32_t> indices = mesh->getIndices();
    const VertexAttribute & positions = mesh->getAttribute("POSITION");

    for (size_t i = 0; i < indices.size(); i += 3) {

      uint32_t i0 = indices[i];
      uint32_t i1 = indices[i+1];
      uint32_t i2 = indices[i+2];
      
      Math::Vector<3, float> p0 = positions.value[i0].vec3;
      Math::Vector<3, float> p1 = positions.value[i1].vec3;
      Math::Vector<3, float> p2 = positions.value[i2].vec3;

      btMesh->addTriangle(btVector3(p0(0), p0(1), p0(2)),
			  btVector3(p1(0), p1(1), p1(2)),
			  btVector3(p2(0), p2(1), p2(2)));
      
    }
    
  }

  struct BoxShapeData {
    
    BoxShapeData(Math::Vector<3> c) : corner(c) {};
    Math::Vector<3> corner;
    
  };

  struct SphereShapeData {

    SphereShapeData(double r) : radius(r) {};
    double radius;
    
  };

  struct CapsuleShapeData {

    CapsuleShapeData(double h, double r) : height(h), radius(r) {};
    double height;
    double radius;
    
  };

  struct MeshShapeData {

    MeshShapeData(std::shared_ptr<Mesh> m, std::string name) {
      mesh = m;
      meshName = name;
    }
    std::shared_ptr<Mesh> mesh;
    std::string meshName;
    
  };
  
};

std::shared_ptr<PhysicsObject> physutil::loadPhysicsObject(std::shared_ptr<config::NodeCompound> data, Transform<double> transform, const std::unordered_map<std::string, LoadingResource> & attachedResources) {

  using namespace config;
  using namespace physutil;

  double mass = data->getNode<double>("mass")->getElement(0);

  std::string shapeName(data->getNode<char>("shapeName")->getRawData());

  btCollisionShape * shape = nullptr;

  std::any shapeData;
  
  if (shapeName == "box") {

    std::cout << "Loading BoxShape " << std::endl;
    double * s = data->getNode<double>("boxSize")->getRawData().get();
    Math::Vector<3, double> svec(s);

    shapeData = BoxShapeData(svec);
    
    svec = Math::compMultiply(svec, transform.scale);
    shape = new btBoxShape(btVector3(svec[0], svec[1], svec[2]));
    
    
  } else if (shapeName == "sphere") {

    double radius = data->getNode<double>("radius")->getElement(0);
    shape = new btSphereShape(radius);
    shapeData = SphereShapeData(radius);
    
  } else if (shapeName == "capsule") {

    double radius = data->getNode<double>("radius")->getElement(0);
    double height = data->getNode<double>("height")->getElement(0);
    
    shape = new btCapsuleShapeZ(radius, height);

    shapeData = CapsuleShapeData(height, radius);
    
  } else if (shapeName == "cylinder") {

    double radius = data->getNode<double>("radius")->getElement(0);
    double height = data->getNode<double>("height")->getElement(0);

    shapeData = CapsuleShapeData(height, radius);

    btVector3 vec(radius, radius, height/2);
    
    shape = new btCylinderShapeZ(vec);
    
  } else if (shapeName == "static_mesh") {

    std::string resName(data->getNode<char>("name")->getRawData());
    std::cout << "Looking for attached resource with name " << resName << std::endl;

    if (attachedResources.find(resName) == attachedResources.end())
      throw dbg::trace_exception("No matching attached resource found.");

    LoadingResource res = attachedResources.at(resName);
    std::shared_ptr<Mesh> mesh = std::dynamic_pointer_cast<Mesh>(res->location);

    shapeData = MeshShapeData(mesh, resName);

    btTriangleMesh * btMesh = new btTriangleMesh();

    transferMeshDataToBullet(mesh, btMesh);
    
    shape = new btBvhTriangleMeshShape(btMesh, true);

  }

  if (!shape)
    throw dbg::trace_exception("Unknown collision shape");

  std::shared_ptr<PhysicsObject> obj = std::make_shared<PhysicsObject>(mass, transform, shape);
  obj->setShapeType(shapeName);
  obj->setShapeData(shapeData);
  return obj;
  
}

std::shared_ptr<config::NodeCompound> physutil::savePhysicsObject(std::shared_ptr<PhysicsObject> obj) {

  std::shared_ptr<config::NodeCompound> comp = std::make_shared<config::NodeCompound>();

  std::string shapeType = obj->getShapeType();
  comp->addChild("shapeName", std::make_shared<config::Node<char>>(shapeType.length(), shapeType.c_str()));

  if (shapeType == "box") {

    BoxShapeData bData = std::any_cast<BoxShapeData>(obj->getShapeData());
    comp->addChild("boxSize", std::make_shared<config::Node<double>>(3, bData.corner.getData()));
    
  } else if (shapeType == "sphere") {

    SphereShapeData sData = std::any_cast<SphereShapeData>(obj->getShapeData());
    comp->addChild("radius", std::make_shared<config::Node<double>>(1, &sData.radius));
    
  } else if (shapeType == "capsule" || shapeType == "cylinder") {

    CapsuleShapeData cData = std::any_cast<CapsuleShapeData>(obj->getShapeData());
    comp->addChild("height", std::make_shared<config::Node<double>>(1, &cData.height));
    comp->addChild("radius", std::make_shared<config::Node<double>>(1, &cData.radius));
    
  } else if (shapeType == "static_mesh") {

    MeshShapeData mData = std::any_cast<MeshShapeData>(obj->getShapeData());
    comp->addChild("name", std::make_shared<config::Node<char>>(mData.meshName.length(), mData.meshName.c_str()));
    
  }

  double mass = obj->getMass();
  comp->addChild("mass", std::make_shared<config::Node<double>>(1, &mass));

  return comp;
  
}

PhysicsContext::RaycastResult PhysicsContext::performRaycastClosest(Math::Vector<3> origin, Math::Vector<3> direction, double maxLength) {

  btVector3 start(origin[0], origin[1], origin[2]);
  Math::Vector<3> tmpEnd = origin + maxLength * direction;
  btVector3 end(tmpEnd[0], tmpEnd[1], tmpEnd[2]);
  
  btCollisionWorld::ClosestRayResultCallback callback(start, end);

  this->dynamicsWorld->rayTest(start, end, callback);

  RaycastResult res;
  
  if (callback.hasHit()) {

    btVector3 wPos = callback.m_hitPointWorld;
    Math::Vector<3> pos({wPos.x(), wPos.y(), wPos.z()});

    res.distance = (pos - origin).length();
    res.object = (PhysicsObject *) callback.m_collisionObject->getUserPointer();
    
  } else {
    res.distance = std::numeric_limits<double>::infinity();
    res.object = nullptr;
  }

  return res;
  
}
