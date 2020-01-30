#ifndef RENDERELEMENT_H
#define RENDERELEMENT_H

#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>

#include "model.h"
#include "texture.h"
#include "shader.h"
#include "dynamicbuffer.h"
#include "util/math/quaternion.h"
#include "memorytransferhandler.h"
#include "material.h"

class Viewport;

struct UniformBufferObject {

    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;

};


class RenderElement : public MemoryTransferer {

    public:

        struct Transform {
            glm::vec3 position;
            Math::Quaternion<float> qRot;
            float scale;
        };

        struct Instance {
            uint32_t id;
        };
        RenderElement(Viewport * view, std::shared_ptr<Model> model, std::shared_ptr<Material> mat, int scSize, Transform & initTrasnsform);
        virtual ~RenderElement();

        glm::mat4 getTransformationMatrix(Transform & instance);

        void render(VkCommandBuffer & cmdBuffer, uint32_t frameIndex);
        void renderShaderless(VkCommandBuffer & buffer, uint32_t frameIndex);

        Instance addInstance(Transform & trans);
        void updateInstance(Instance & instance, Transform & trans);
        void deleteInstance(Instance & instance);

        void createUniformBuffers(int scSize);
        void destroyUniformBuffers(const vkutil::SwapChain & swapchain);

        void recreateResources(VkRenderPass & renderPass, int scSize, const vkutil::SwapChain & swapchain);

        void updateUniformBuffer(UniformBufferObject & obj, uint32_t frameIndex);

        std::vector<VkDescriptorSet> & getDescriptorSets();
        std::vector<VmaAllocation> & getMemories();

        bool needsDrawCmdUpdate();

        void recordTransfer(VkCommandBuffer & cmdBuffer);
        bool reusable();

        Shader * getShader();

    protected:

    private:

        const vkutil::VulkanState & state;

        struct InstanceInfo {

            uint32_t id;
            uint32_t pos;

        };

        std::shared_ptr<Model> model;
        std::shared_ptr<Shader> shader;
        std::vector<std::shared_ptr<Texture>> texture;

        VkDescriptorPool descPool;
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;

        std::vector<VkDescriptorSet> descriptorSets;
        std::vector<VkBuffer> uniformBuffers;
        std::vector<VmaAllocation> uniformBuffersMemory;

        DynamicBuffer<glm::mat4> * instanceBuffer;
        std::vector<glm::mat4> instanceTransforms;
        std::unordered_map<uint32_t, InstanceInfo> instances;
        std::vector<Transform> transforms;
        uint32_t instanceCount;

        std::mutex transformBufferMutex;

        bool instanceBufferDirty;
        bool instanceCountUpdated;

        RenderElement(Viewport * view, std::shared_ptr<Model> model, std::shared_ptr<Shader> shader, std::vector<std::shared_ptr<Texture>> texture, int scSize, Transform & initTransform);
        void markBufferDirty();


};

#endif // RENDERELEMENT_H
