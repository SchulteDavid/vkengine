#ifndef LEVEL_H
#define LEVEL_H

#include <memory>

#include "structure/structure.h"

#include <mathutils/vector.h>
#include <mathutils/quaternion.h>

class Level {

    public:

        struct placement_t {

            Math::Vector<3, double> pos;
            Math::Quaternion<double> rot;
            Math::Vector<3, double> scale;

            std::shared_ptr<Structure> strc;

        };

        Level();
        virtual ~Level();

    protected:

    private:



};

#endif // LEVEL_H
