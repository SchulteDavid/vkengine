#include "mesh.h"

#include <ply.hpp>

Mesh::Mesh(std::vector<Model::Vertex> verts, std::vector<uint16_t> indices) {

    this->verts = verts;
    this->indices = indices;

}

Mesh::~Mesh() {



}

std::vector<Model::Vertex> & Mesh::getVerts() {
    return verts;
}

std::vector<uint16_t> & Mesh::getIndices() {
    return indices;
}

std::shared_ptr<Mesh> Mesh::withTransform(std::shared_ptr<Mesh> mesh, Math::Matrix<4,4,float> m) {

    using namespace Math;

    std::vector<Model::Vertex> newVerts;

    for (Model::Vertex v : mesh->verts) {

        Vector<4, float> p(v.pos.x, v.pos.y, v.pos.z, 1);
        Vector<4, float> n(v.normal.x, v.normal.y, v.normal.z, 0);
        Vector<4, float> t(v.tangent.x, v.tangent.y, v.tangent.z, 0);

        Vector<4, float> p2 = m * p;
        n = m * n;
        t = m * t;

        Model::Vertex nVert;

        nVert.pos = glm::vec3(p2[0] / p2[3], p2[1] / p2[3], p2[2] / p2[3]);
        nVert.normal = glm::vec3(n[0], n[1], n[2]);
        nVert.tangent = glm::vec3(t[0], t[1], t[2]);
        nVert.uv = v.uv;
        nVert.matIndex = v.matIndex;

        newVerts.push_back(nVert);

    }

    return std::shared_ptr<Mesh>(new Mesh(newVerts, mesh->indices));

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
