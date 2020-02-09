#ifndef LEVEL_H
#define LEVEL_H

#include <memory>
#include <unordered_map>

#include "structure/structure.h"
#include "resources/resource.h"

#include <mathutils/vector.h>
#include <mathutils/quaternion.h>

class World;
class Viewport;

class Level : public Resource {

    public:

        struct Placement;

        Level();
        virtual ~Level();


        void addElement(std::shared_ptr<Structure> strc, Math::Vector<3> pos, Math::Quaternion<double> rot, Math::Vector<3> scale, double mass);
        void addElement(std::shared_ptr<Structure> strc, Placement & p);
        void applyToWorld(std::shared_ptr<World> world, Viewport * view);


    protected:

    private:

        std::unordered_map<std::shared_ptr<Structure>, std::vector<Placement>> placements;
        std::vector<std::shared_ptr<Structure>> structs;

};

class LevelLoader : public ResourceLoader<Level> {

    public:
        std::shared_ptr<ResourceUploader<Level>> loadResource(std::string fname);

};

#endif // LEVEL_H
