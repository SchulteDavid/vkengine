#ifndef MODEL_H
#define MODEL_H

#include <glm/glm.hpp>

#include "indexbuffer.h"
#include "vertexbuffer.h"
#include <string>

#include "resources/resource.h"
#include "resources/resourceloader.h"

class Mesh;

class Model : public Resource {

    public:

        struct Vertex {

            glm::vec3 pos;
            glm::vec3 normal;
            glm::vec3 tangent;
            glm::vec2 uv;
            int32_t matIndex;

            static std::vector<VkVertexInputBindingDescription> getBindingDescription();
            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

        };

        Model(const vkutil::VulkanState & state, std::vector<Vertex> & verts, std::vector<uint16_t> & indices);
        Model(const vkutil::VulkanState & state, std::shared_ptr<Mesh> mesh);
        virtual ~Model();

        void uploadToGPU(const VkDevice & device, const VkCommandPool & commandPool, const VkQueue & q);

        void bindForRender(VkCommandBuffer & cmdBuffer);
        int getIndexCount();

        virtual std::vector<VkVertexInputBindingDescription> getBindingDescription();
        virtual std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

        static Model * loadFromFile(const vkutil::VulkanState & state, std::string fname);

    protected:

    private:

        VertexBuffer<uint8_t> * vBuffer;
        IndexBuffer<uint16_t> * iBuffer;

        int vCount;
        int iCount;

};

class ModelUploader : public ResourceUploader<Model> {

    public:
        ModelUploader(const vkutil::VulkanState & state, Model * model);

        Model * uploadResource();
        bool uploadReady();

    private:

        const vkutil::VulkanState & state;
        Model * model;

};

class ModelLoader : public ResourceLoader<Model> {

    public:

        ModelLoader(const vkutil::VulkanState & state);

        std::shared_ptr<ResourceUploader<Model>> loadResource(std::string fname);

    private:

        const vkutil::VulkanState & state;

};

#endif // MODEL_H
