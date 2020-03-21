#ifndef MESHHELPER_H
#define MESHHELPER_H

#include "../render/model.h"
#include <mathutils/vector.h>

class MeshHelper
{
    public:

        struct ModelInfo {

            std::vector<Model::Vertex> verts;
            std::vector<uint16_t> indices;

        };


        static void computeTangents(std::vector<Model::Vertex> & verts, std::vector<uint32_t> & indices);
        static void computeTangents(std::vector<Model::Vertex> & verts, std::vector<uint16_t> & indices);
        static ModelInfo createHexagonPlane(int amount, float radius);
        static ModelInfo createHexagonFromCenter(Math::Vector<3, float> center, Math::Vector<3, float> normal, float radius);
        static void mergeMeshData(ModelInfo & data1, ModelInfo & data2, float precision);
        static float noise(float x, float y);

    protected:

    private:
};

#endif // MESHHELPER_H
