#include "level.h"

#include "render/viewport.h"
#include "world/world.h"
#include <configloading.h>
#include <iostream>

#include <bullet/btBulletDynamicsCommon.h>

using namespace Math;

struct Level::Placement  {

    Math::Vector<3, double> pos;
    Math::Quaternion<double> rot;
    Math::Vector<3, double> scale;

    double mass;
    double angularFactor;
    btCollisionShape * collisionShape;

    std::string entityType;

    bool placeStructure;

};

Level::Level() {

    this->placements = std::unordered_map<std::shared_ptr<Structure>, std::vector<Placement>>();
    this->structs = std::vector<std::shared_ptr<Structure>>();

}

Level::~Level() {

}

void Level::addElement(std::shared_ptr<Structure> strc, Vector<3> pos, Quaternion<double> rot, Vector<3> scale, double mass) {

    if (this->placements.find(strc) == this->placements.end()) {

        std::vector<Placement> vec(1);
        vec[0].pos = pos;
        vec[0].rot = rot;
        vec[0].scale = scale;
        vec[0].mass = mass;

        this->placements[strc] = vec;

        structs.push_back(strc);

        return;

    }

    Placement p;
    p.pos = pos;
    p.rot = rot;
    p.scale = scale;
    p.mass = mass;

    this->placements[strc].push_back(p);

}

void Level::addElement(std::shared_ptr<Structure> strc, Placement & p) {

    if (this->placements.find(strc) == placements.end()) {
        std::vector<Placement> vec(1);
        vec[0] = p;

        placements[strc] = vec;

        structs.push_back(strc);

        return;
    }

    placements[strc].push_back(p);
}

void Level::applyToWorld(std::shared_ptr<World> world, Viewport * view) {

    std::cout << "Level, internal " << this->structs.size() << std::endl;

    for (std::shared_ptr<Structure> strc : structs) {

        RenderElement::Transform initTrans;
        initTrans.scale = 0.0;
        initTrans.position = Vector<4,float>();
        initTrans.qRot = Quaternion<float>();
        std::shared_ptr<RenderElement> rElem(RenderElement::buildRenderElement(view, strc, initTrans));

        view->addRenderElement(rElem);

        for (Placement & p : placements[strc]) {

            std::shared_ptr<PhysicsObject> physObj(new PhysicsObject(p.mass, p.pos, p.rot, p.collisionShape));

            physObj->setAngularFactor(p.angularFactor);

            std::shared_ptr<Entity> entity;
            if (!p.placeStructure) {
                entity = Entity::buildEntityFromType(p.entityType, nullptr, (RenderElement::Instance) {0}, physObj);
            } else {
                RenderElement::Transform trans;
                trans.scale = p.scale[0];
                trans.qRot = Quaternion<float>(p.rot.a, p.rot.b, p.rot.c, p.rot.d);
                trans.position = Vector<4,float>(p.pos[0], p.pos[1], p.pos[2], 0);

                RenderElement::Instance instance = rElem->addInstance(trans);
                entity = Entity::buildEntityFromType(p.entityType, rElem, instance, physObj);
            }

            world->addEntity(entity);

        }

    }

}

struct LoadingPlacement {

    LoadingResource strc;
    Level::Placement p;

};

class LevelUploader : public ResourceUploader<Level> {

    public:
        LevelUploader(std::vector<LoadingPlacement> pl) {
            this->placements = pl;
        }

        Level * uploadResource() {

            Level * lvl = new Level();
            for (LoadingPlacement p : placements) {

                std::shared_ptr<Structure> strc = std::dynamic_pointer_cast<Structure>(p.strc->location);
                if (!strc)
                    throw dbg::trace_exception("Error casting Structure");
                lvl->addElement(strc, p.p);

            }

            return lvl;

        }

        bool uploadReady() {
            bool b = true;
            for (LoadingPlacement p : placements) {

                b &= p.strc->status.isUseable;

            }
            return b;
        }

    private:
        std::vector<LoadingPlacement> placements;

};

void loadPlacementTransform(std::shared_ptr<config::NodeCompound> trans, Level::Placement & p) {

    p.pos = Vector<3>(trans->getNode<double>("position")->getRawData().get());
    p.scale = Vector<3>(trans->getNode<double>("scale")->getRawData().get());
    double * tmp = trans->getNode<double>("rotation")->getRawData().get();
    p.rot = Quaternion<double>(tmp[0], tmp[1], tmp[2], tmp[3]);

}

void loadPlacementPhysics(std::shared_ptr<config::NodeCompound> physics, Level::Placement & p) {

    p.mass = physics->getNode<double>("mass")->getElement(0);

    /// TODO: add loading of different collision shapes.

    std::shared_ptr<config::NodeCompound> collision = physics->getNodeCompound("collision");

    std::string type(collision->getNode<char>("type")->getRawData());
    if (!type.compare("box")) {
        std::shared_ptr<config::Node<double>> s = collision->getNode<double>("size");
        p.collisionShape = new btBoxShape(btVector3(s->getElement(0), s->getElement(1), s->getElement(2)));
    } else if (!type.compare("sphere")){
        double radius = collision->getNode<double>("radius")->getElement(0);
        p.collisionShape = new btSphereShape(radius);
    } else if (!type.compare("capsule")) {
        double radius = collision->getNode<double>("radius")->getElement(0);
        double height = collision->getNode<double>("height")->getElement(0);
        p.collisionShape = new btCapsuleShapeZ(radius, height);
    }else {
        p.collisionShape = new btBoxShape(btVector3(1,1,1));
    }

    try {

        double fac = physics->getNode<double>("angularFactor")->getElement(0);
        p.angularFactor = fac;

    } catch (...) {
        p.angularFactor = 1.0;
    }

}

std::shared_ptr<ResourceUploader<Level>> LevelLoader::loadResource(std::string fname) {

    using namespace config;

    std::shared_ptr<NodeCompound> root = config::parseFile(fname);

    std::shared_ptr<Node<std::shared_ptr<NodeCompound>>> elements = root->getNode<std::shared_ptr<NodeCompound>>("elements");

    std::vector<LoadingResource> structureResources(elements->getElementCount());
    std::vector<LoadingPlacement> tmpPlacements;

    for (unsigned int i = 0; i < elements->getElementCount(); ++i) {

        std::shared_ptr<NodeCompound> elem = elements->getElement(i);

        std::string strcName(elem->getNode<char>("structure")->getRawData());
        std::string entityType(elem->getNode<char>("entityType")->getRawData());

        LoadingResource strcRes = this->loadDependency("Structure", strcName);
        structureResources[i] = strcRes;

        std::shared_ptr<NodeCompound> trans = elem->getNodeCompound("transform");
        std::shared_ptr<NodeCompound> physics = elem->getNodeCompound("physics");


        bool placeStructure;
        try {
            placeStructure = elem->getNode<int32_t>("placeStructure")->getElement(0);
        } catch (...) {
            placeStructure = true;
        }

        Level::Placement p;
        loadPlacementTransform(trans, p);
        loadPlacementPhysics(physics, p);

        p.entityType = entityType;
        p.placeStructure = placeStructure;

        LoadingPlacement ldPlace;
        ldPlace.p = p;
        ldPlace.strc = strcRes;

        tmpPlacements.push_back(ldPlace);

    }

    return std::shared_ptr<ResourceUploader<Level>>((ResourceUploader<Level> *) new LevelUploader(tmpPlacements));

}
