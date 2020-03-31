#ifndef MARCHINGCUBES_H
#define MARCHINGCUBES_H

#include <memory>
#include <functional>

#include "util/mesh.h"

#include <mathutils/vector.h>

std::shared_ptr<Mesh> buildMeshFromFunction(std::function<double(double, double, double)> f, Math::Vector<3, float> center, Math::Vector<3, float> extend, double isoValue, int divCount);
std::future<std::shared_ptr<Mesh>> generateBackground(std::function<double(double, double, double)> f, Math::Vector<3, float> center, Math::Vector<3, float> extend, double isoValue, int divCount);
void testOctree();

#endif // MARCHINGCUBES_H
