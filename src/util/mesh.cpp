#include "mesh.h"

#include <ply.hpp>

using namespace Math;

Mesh::Mesh(std::vector<Model::Vertex> verts, std::vector<uint16_t> indices) {

    this->verts = verts;
    this->indices = indices;


    VertexAttribute positionAttr;
    positionAttr.type = ATTRIBUTE_VEC3;
    positionAttr.value = std::vector<VertexAttribute::VertexAttributeData>(verts.size());

    VertexAttribute normalAttr;
    normalAttr.type = ATTRIBUTE_VEC3;
    normalAttr.value = std::vector<VertexAttribute::VertexAttributeData>(verts.size());

    VertexAttribute uvAttr;
    uvAttr.type = ATTRIBUTE_VEC2;
    uvAttr.value = std::vector<VertexAttribute::VertexAttributeData>(verts.size());

    VertexAttribute tangentAttr;
    tangentAttr.type = ATTRIBUTE_VEC3;
    tangentAttr.value = std::vector<VertexAttribute::VertexAttributeData>(verts.size());

    VertexAttribute matAttr;
    matAttr.type = ATTRIBUTE_INT;
    matAttr.value = std::vector<VertexAttribute::VertexAttributeData>(verts.size());

    for (unsigned int i = 0; i < verts.size(); ++i) {

        float tmp[3] = {verts[i].pos.x, verts[i].pos.y, verts[i].pos.z};
        positionAttr.value[i].vec3 = Vector<3, float>(tmp);

        tmp[0] = verts[i].normal.x;
        tmp[1] = verts[i].normal.y;
        tmp[2] = verts[i].normal.z;
        normalAttr.value[i].vec3 = Vector<3, float>(tmp);

        tmp[0] = verts[i].tangent.x;
        tmp[1] = verts[i].tangent.y;
        tmp[2] = verts[i].tangent.z;
        tangentAttr.value[i].vec3 = Vector<3, float>(tmp);

        tmp[0] = verts[i].uv.x;
        tmp[1] = verts[i].uv.y;
        uvAttr.value[i].vec2 = Vector<2, float>(tmp);

        matAttr.value[i].i = verts[i].matIndex;

    }

    this->attributes["POSITION"] = positionAttr;
    this->attributes["NORMAL"] = normalAttr;
    this->attributes["TEXCOORD_0"] = uvAttr;
    this->attributes["TANGENT"] = tangentAttr;
    this->attributes["MATERIAL_INDEX"] = matAttr;

    for (auto it : attributes) {

        logger(std::cout) << it.first << " -> " << it.second.type << std::endl;

    }

}

Mesh::Mesh(std::unordered_map<std::string, VertexAttribute> attributes, std::vector<uint16_t> indices) {

    this->attributes = attributes;
    this->indices = indices;

}

Mesh::~Mesh() {



}

std::vector<Model::Vertex> Mesh::getVerts() {
    //return verts;

    std::vector<Model::Vertex> verts(this->attributes["POSITION"].value.size());

    for (unsigned int i = 0; i < this->attributes["POSITION"].value.size(); ++i) {

        verts[i].pos = glm::vec3(attributes["POSITION"].value[i].vec3[0], attributes["POSITION"].value[i].vec3[1], attributes["POSITION"].value[i].vec3[2]);
        verts[i].normal = glm::vec3(attributes["NORMAL"].value[i].vec3[0], attributes["NORMAL"].value[i].vec3[1], attributes["NORMAL"].value[i].vec3[2]);
        verts[i].tangent = glm::vec3(attributes["TANGENT"].value[i].vec3[0], attributes["TANGENT"].value[i].vec3[1], attributes["TANGENT"].value[i].vec3[2]);
        verts[i].uv = glm::vec2(attributes["TEXCOORD_0"].value[i].vec2[0], attributes["TEXCOORD_0"].value[i].vec2[1]);
        verts[i].matIndex = attributes["MATERIAL_INDEX"].value[i].i;

    }

    return verts;

}

std::vector<uint16_t> & Mesh::getIndices() {
    return indices;
}

std::shared_ptr<Mesh> Mesh::withTransform(std::shared_ptr<Mesh> mesh, Math::Matrix<4,4,float> m) {

    using namespace Math;

    //std::vector<Model::Vertex> newVerts;
    std::unordered_map<std::string, VertexAttribute> newAttributes;

    for (auto it : mesh->attributes) {

        std::vector<VertexAttribute::VertexAttributeData> data(it.second.value.size());
        for (unsigned int i = 0; i < it.second.value.size(); ++i) {
            data[i] = it.second.value[i];
        }

        VertexAttribute attr;
        attr.type = it.second.type;
        attr.value = data;

        newAttributes[it.first] = attr;

    }

    for (unsigned int i = 0; i < mesh->attributes["POSITION"].value.size(); ++i) {

        Vector<3, float> pos = mesh->attributes["POSITION"].value[i].vec3;

        Vector<4, float> p(pos[0], pos[1], pos[2], 1);
        Vector<4, float> n(mesh->attributes["NORMAL"].value[i].vec3);
        Vector<4, float> t(mesh->attributes["TANGENT"].value[i].vec3);

        Vector<4,float> p2 = m * p;
        n = m * n;
        t = m * t;

        //Model::Vertex nVert;

        //nVert.pos = glm::vec3(p[0] / p[3], p[1] / p[3], p[2] / p[3]);
        newAttributes["POSITION"].value[i].vec3 = Vector<3, float>(p2) / p2[3];
        newAttributes["NORMAL"].value[i].vec3 = Vector<3, float>(n);
        //logger(std::cout) << p << std::endl;
        //logger(std::cout) << newAttributes["POSITION"].value[i].vec3 << std::endl;
        newAttributes["TANGENT"].value[i].vec3 = Vector<3, float>(t);
        //nVert.normal = glm::vec3(n[0], n[1], n[2]);
        //nVert.tangent = glm::vec3(t[0], t[1], t[2]);
        //nVert.uv = v.uv;
        //nVert.matIndex = v.matIndex;

        //newVerts.push_back(nVert);

    }

    return std::shared_ptr<Mesh>(new Mesh(newAttributes, mesh->indices));

}

void Mesh::setMaterialIndex(int32_t index) {
    for (unsigned int i = 0; i < verts.size(); ++i) {
        verts[i].matIndex = index;
    }
}

std::shared_ptr<Mesh> operator*(Math::Matrix<4,4,float> m, std::shared_ptr<Mesh> mesh) {

    return Mesh::withTransform(mesh, m);

}

std::shared_ptr<Mesh> Mesh::loadFromFile(std::string fname) {

    PlyFile * plyFile = new PlyFile(fname);
    int indexCount;
    int * indexData = plyFile->getIndexData(&indexCount);

    int vertexCount;
    float * vertexData = plyFile->getVertexData(&vertexCount);

    std::vector<Model::Vertex> vertices(vertexCount);

    for (int i = 0; i < vertexCount; ++i) {

        vertices[i].pos = glm::vec3(vertexData[i*11], vertexData[i*11+1], vertexData[i*11+2]);
        vertices[i].normal = glm::vec3(vertexData[i*11+3], vertexData[i*11+4], vertexData[i*11+5]);
        vertices[i].uv = glm::vec2(vertexData[i*11+6], vertexData[i*11+7]);
        vertices[i].matIndex = 0;

    }

    std::vector<uint16_t> indices(indexCount);

    for (int i = 0; i < indexCount; ++i)
        indices[i] = indexData[i];

    return std::shared_ptr<Mesh>(new Mesh(vertices, indices));

}

std::shared_ptr<Mesh> Mesh::merge(std::shared_ptr<Mesh> m1, std::shared_ptr<Mesh> m2) {

    if (!m1) return m2;
    if (!m2) return m1;

    std::vector<Model::Vertex> verts;
    std::vector<uint16_t> indices;

    for (Model::Vertex v : m1->verts) {
        verts.push_back(v);
    }

    uint16_t m1Count = verts.size();

    for (Model::Vertex v : m2->verts) {
        verts.push_back(v);
    }

    for (uint16_t i : m1->indices) {
        indices.push_back(i);
    }

    for (uint16_t i : m2->indices) {
        indices.push_back(i+m1Count);
    }

    return std::shared_ptr<Mesh>(new Mesh(verts, indices));

}
