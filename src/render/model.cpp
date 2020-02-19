#include "model.h"

#include "util/mesh.h"
#include "util/meshhelper.h"

#include <ply.hpp>
#include <iostream>

std::vector<VkVertexInputBindingDescription> Model::Vertex::getBindingDescription() {

    VkVertexInputBindingDescription description = {};
    description.binding = 0;
    description.stride = sizeof(Vertex);
    description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputBindingDescription instances = {};
    instances.binding = 1;
    instances.stride = sizeof(glm::mat4);
    instances.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    std::vector<VkVertexInputBindingDescription> bindings = {description, instances};

    return bindings;

}

std::vector<VkVertexInputAttributeDescription> Model::Vertex::getAttributeDescriptions() {

    std::vector<VkVertexInputAttributeDescription> descriptions(9);

    descriptions[0].binding = 0;
    descriptions[0].location = 0;
    descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions[0].offset = offsetof(Vertex, pos);

    descriptions[1].binding = 0;
    descriptions[1].location = 1;
    descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions[1].offset = offsetof(Vertex, normal);

    descriptions[2].binding = 0;
    descriptions[2].location = 2;
    descriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions[2].offset = offsetof(Vertex, tangent);

    descriptions[3].binding = 0;
    descriptions[3].location = 3;
    descriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
    descriptions[3].offset = offsetof(Vertex, uv);

    descriptions[4].binding = 0;
    descriptions[4].location = 4;
    descriptions[4].format = VK_FORMAT_R32_SINT;
    descriptions[4].offset = offsetof(Vertex, matIndex);

    descriptions[5].binding = 1;
    descriptions[5].location = 5;
    descriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    descriptions[5].offset = 0;

    descriptions[6].binding = 1;
    descriptions[6].location = 6;
    descriptions[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    descriptions[6].offset = sizeof(glm::vec4);

    descriptions[7].binding = 1;
    descriptions[7].location = 7;
    descriptions[7].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    descriptions[7].offset = 2*sizeof(glm::vec4);

    descriptions[8].binding = 1;
    descriptions[8].location = 8;
    descriptions[8].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    descriptions[8].offset = 3*sizeof(glm::vec4);

    return descriptions;

}

std::vector<uint8_t> convertToByteBuffer(std::vector<Model::Vertex> & verts) {

    std::vector<uint8_t> data(sizeof(Model::Vertex) * verts.size());

    uint32_t stride = sizeof(Model::Vertex);
    uint32_t offset = 0;

    for (unsigned int i = 0; i < verts.size(); ++i) {

        offset = offsetof(Model::Vertex, pos);
        *((float*) &data.data()[i*stride+offset])   = verts[i].pos.x;
        *((float*) &data.data()[i*stride+offset+1]) = verts[i].pos.y;
        *((float*) &data.data()[i*stride+offset+2]) = verts[i].pos.z;

        offset = offsetof(Model::Vertex, normal);
        *((float*) &data.data()[i*stride+offset])   = verts[i].normal.x;
        *((float*) &data.data()[i*stride+offset+1]) = verts[i].normal.y;
        *((float*) &data.data()[i*stride+offset+2]) = verts[i].normal.z;

        offset = offsetof(Model::Vertex, tangent);
        *((float*) &data.data()[i*stride+offset])   = verts[i].tangent.x;
        *((float*) &data.data()[i*stride+offset+1]) = verts[i].tangent.y;
        *((float*) &data.data()[i*stride+offset+2]) = verts[i].tangent.z;

        offset = offsetof(Model::Vertex, uv);
        *((float*) &data.data()[i*stride+offset])   = verts[i].uv.x;
        *((float*) &data.data()[i*stride+offset+1]) = verts[i].uv.y;

        offset = offsetof(Model::Vertex, matIndex);
        *((int32_t*) &data.data()[i*stride+offset])   = verts[i].matIndex;

    }

    return data;

}

Model::Model(const vkutil::VulkanState & state, std::vector<Vertex> & verts, std::vector<uint16_t> & indices) {

    MeshHelper::computeTangents(verts, indices);

    this->vBuffer = (VertexBuffer<uint8_t> *) new VertexBuffer<Vertex>(state, verts);
    this->iBuffer = new IndexBuffer<uint16_t>(state, indices);

    this->vCount = verts.size();
    this->iCount = indices.size();

}

Model::Model(const vkutil::VulkanState & state, std::shared_ptr<Mesh> mesh) { // : Model(state, mesh->getVerts(), mesh->getIndices()) {

    std::vector<Mesh::InterleaveElement> elements(5);

    elements[0].attributeName = "POSITION";
    elements[0].offset = offsetof(Vertex, pos);

    elements[1].attributeName = "NORMAL";
    elements[1].offset = offsetof(Vertex, normal);

    elements[2].attributeName = "TANGENT";
    elements[2].offset = offsetof(Vertex, tangent);

    elements[3].attributeName = "TEXCOORD_0";
    elements[3].offset = offsetof(Vertex, uv);

    elements[4].attributeName = "MATERIAL_INDEX";
    elements[4].offset = offsetof(Vertex, matIndex);


    std::vector<uint8_t> meshData = mesh->getInterleavedData(elements, sizeof(Vertex));

    this->vBuffer = new VertexBuffer<uint8_t>(state, meshData);
    this->iBuffer = new IndexBuffer<uint16_t>(state, mesh->getIndices());

    this->vCount = mesh->getVertexCount();
    this->iCount = mesh->getIndices().size();

}

Model::~Model() {

    std::cout << "Deleting model" << std::endl;

    delete iBuffer;
    delete vBuffer;

}

void Model::bindForRender(VkCommandBuffer & cmdBuffer) {

    vBuffer->bindForRender(cmdBuffer);
    iBuffer->bindForRender(cmdBuffer);

}

int Model::getIndexCount() {
    return iCount;
}

Model * Model::loadFromFile(const vkutil::VulkanState & state, std::string fname) {

    /*PlyFile * plyFile = new PlyFile(fname);
    int indexCount;
    int * indexData = plyFile->getIndexData(&indexCount);

    int vertexCount;
    float * vertexData = plyFile->getVertexData(&vertexCount);

    std::vector<Vertex> vertices(vertexCount);

    for (int i = 0; i < vertexCount; ++i) {

        vertices[i].pos = glm::vec3(vertexData[i*11], vertexData[i*11+1], vertexData[i*11+2]);
        vertices[i].normal = glm::vec3(vertexData[i*11+3], vertexData[i*11+4], vertexData[i*11+5]);
        vertices[i].uv = glm::vec2(vertexData[i*11+6], vertexData[i*11+7]);

    }

    std::vector<uint16_t> indices(indexCount);

    for (int i = 0; i < indexCount; ++i)
        indices[i] = indexData[i];*/

    std::shared_ptr<Mesh> mesh = Mesh::loadFromFile(fname);
    return new Model(state, mesh);

    //return new Model(state, vertices, indices);

}

void Model::uploadToGPU(const VkDevice & device, const VkCommandPool & commandPool, const VkQueue & q) {

    this->vBuffer->upload(device, commandPool, q);
    this->iBuffer->upload(device, commandPool, q);

}

ModelUploader::ModelUploader(const vkutil::VulkanState & state, Model * model) : state(state) {

    this->model = model;

}

Model * ModelUploader::uploadResource() {

    model->uploadToGPU(state.device, state.transferCommandPool, state.transferQueue);
    return model;

}

std::vector<VkVertexInputBindingDescription> Model::getBindingDescription() {
    return Vertex::getBindingDescription();
}

std::vector<VkVertexInputAttributeDescription> Model::getAttributeDescriptions() {
    return Vertex::getAttributeDescriptions();
}

bool ModelUploader::uploadReady() {
    return true;
}

ModelLoader::ModelLoader(const vkutil::VulkanState & state) : state(state) {

}

std::shared_ptr<ResourceUploader<Model>> ModelLoader::loadResource(std::string fname) {

    Model * model = Model::loadFromFile(state, fname);
    return std::shared_ptr<ResourceUploader<Model>>(new ModelUploader(state, model));

}
