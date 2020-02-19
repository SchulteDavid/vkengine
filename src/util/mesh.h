#ifndef MESH_H
#define MESH_H

#include <vector>
#include <memory>
#include <unordered_map>

#include "render/model.h"
#include <mathutils/matrix.h>

typedef enum VertexAttributeType : uint32_t {

    ATTRIBUTE_NONE,
    ATTRIBUTE_FLOAT=1,
    ATTRIBUTE_VEC2=2,
    ATTRIBUTE_VEC3=3,
    ATTRIBUTE_VEC4=4,
    ATTRIBUTE_INT=5,

} VertexAttributeType ;

class VertexAttribute {

    public:

        union VertexAttributeData {

            float f;
            Math::Vector<2, float> vec2;
            Math::Vector<3, float> vec3;
            Math::Vector<4, float> vec4;
            int32_t i;

            VertexAttributeData() {
            }

            ~VertexAttributeData(){
            }

            VertexAttributeData(const VertexAttributeData & d) {
                memcpy(this, &d, sizeof(VertexAttributeData));
            }

            VertexAttributeData & operator=(const VertexAttributeData & d) {

                memcpy(this, &d, sizeof(VertexAttributeData));
                return *this;

            }

        };

        VertexAttributeType type;
        std::vector<VertexAttributeData> value;

};

class Mesh {

    public:

        struct InterleaveElement {

            std::string attributeName;
            uint32_t offset;

        };

        Mesh(std::vector<Model::Vertex> verts, std::vector<uint16_t> indices);
        Mesh(std::unordered_map<std::string, VertexAttribute> attributes, std::vector<uint16_t> indices);
        virtual ~Mesh();

        std::vector<Model::Vertex> getVerts();
        std::vector<uint16_t> & getIndices();

        const VertexAttribute & getAttribute(std::string name);

        std::vector<uint8_t> getInterleavedData(std::vector<InterleaveElement> elements, uint32_t stride);

        void setMaterialIndex(int32_t index);

        static std::shared_ptr<Mesh> loadFromFile(std::string fname);
        static std::shared_ptr<Mesh> withTransform(std::shared_ptr<Mesh> mesh, Math::Matrix<4,4,float> m);

        static std::shared_ptr<Mesh> merge(std::shared_ptr<Mesh> m1, std::shared_ptr<Mesh> m2);

    protected:

    private:

        std::vector<Model::Vertex> verts;
        std::vector<uint16_t> indices;

        std::unordered_map<std::string, VertexAttribute> attributes;

};

std::shared_ptr<Mesh> operator*(Math::Matrix<4,4,float> m, std::shared_ptr<Mesh> mesh);

#endif // MESH_H
