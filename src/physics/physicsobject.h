#ifndef PHYSICSOBJECT_H
#define PHYSICSOBJECT_H

#include <bullet/btBulletDynamicsCommon.h>

#include <mathutils/quaternion.h>
#include <mathutils/vector.h>

class PhysicsObject {

    public:

        Math::Quaternion<double> rotation;
        Math::Vector<3, double>  position;

        PhysicsObject(double mass, Math::Vector<3,double> pos, Math::Quaternion<double> rot);
        virtual ~PhysicsObject();

        double getMass();
        double getAngularFactor();
        btCollisionShape * getCollisionShape();

        Math::Quaternion<double> getRotation();
        Math::Vector<3, double> getPosition();

        void setRigidBody(btRigidBody * body);

        void synchronize();

    protected:

    private:

        double mass;
        double angularFactor;
        btCollisionShape * collisionShape;
        btRigidBody * rigidBody;


};

#endif // PHYSICSOBJECT_H
