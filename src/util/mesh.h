#ifndef MESH_H
#define MESH_H

#include <vector>
#include <memory>

#include "render/model.h"
#include <mathutils/matrix.h>

class Mesh {

    public:
        Mesh(std::vector<Model::Vertex> verts, std::vector<uint16_t> indices);
        virtual ~Mesh();

        std::vector<Model::Vertex> & getVerts();
        std::vector<uint16_t> & getIndices();

        void setMaterialIndex(int32_t index);

        static std::shared_ptr<Mesh> loadFromFile(std::string fname);
        static std::shared_ptr<Mesh> withTransform(std::shared_ptr<Mesh> mesh, Math::Matrix<4,4,float> m);

        static std::shared_ptr<Mesh> merge(std::shared_ptr<Mesh> m1, std::shared_ptr<Mesh> m2);

    protected:

    private:

        std::vector<Model::Vertex> verts;
        std::vector<uint16_t> indices;

};

std::shared_ptr<Mesh> operator*(Math::Matrix<4,4,float> m, std::shared_ptr<Mesh> mesh);

#endif // MESH_H
