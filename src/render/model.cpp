#include "model.h"

#include "../util/meshhelper.h"

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

    std::vector<VkVertexInputAttributeDescription> descriptions(8);

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

    descriptions[4].binding = 1;
    descriptions[4].location = 4;
    descriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    descriptions[4].offset = 0;

    descriptions[5].binding = 1;
    descriptions[5].location = 5;
    descriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    descriptions[5].offset = sizeof(glm::vec4);

    descriptions[6].binding = 1;
    descriptions[6].location = 6;
    descriptions[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    descriptions[6].offset = 2*sizeof(glm::vec4);

    descriptions[7].binding = 1;
    descriptions[7].location = 7;
    descriptions[7].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    descriptions[7].offset = 3*sizeof(glm::vec4);

    return descriptions;

}

Model::Model(const vkutil::VulkanState & state, std::vector<Vertex> & verts, std::vector<uint16_t> & indices) {

    MeshHelper::computeTangents(verts, indices);

    this->vBuffer = new VertexBuffer<Vertex>(state, verts);
    this->iBuffer = new IndexBuffer<uint16_t>(state, indices);

    this->vCount = verts.size();
    this->iCount = indices.size();

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

    PlyFile * plyFile = new PlyFile(fname);
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
        indices[i] = indexData[i];

    return new Model(state, vertices, indices);

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

bool ModelUploader::uploadReady() {
    return true;
}

ModelLoader::ModelLoader(const vkutil::VulkanState & state) : state(state) {

}

std::shared_ptr<ResourceUploader<Model>> ModelLoader::loadResource(std::string fname) {

    Model * model = Model::loadFromFile(state, fname);
    return std::shared_ptr<ResourceUploader<Model>>(new ModelUploader(state, model));

}
