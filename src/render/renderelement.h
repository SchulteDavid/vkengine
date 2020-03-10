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
#include <mathutils/quaternion.h>
#include "memorytransferhandler.h"
#include "material.h"
#include "structure/structure.h"

class Viewport;

struct UniformBufferObject {

    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;

};


class RenderElement : public MemoryTransferer {

    public:

        struct Transform {
            Math::Vector<4, float> position;
            Math::Quaternion<float> qRot;
            float scale;
        };

        struct Instance {
            uint32_t id;
        };
        RenderElement(Viewport * view, std::shared_ptr<Model> model, std::shared_ptr<Material> mat, int scSize, Transform & initTrasnsform);
        RenderElement(Viewport * view, std::shared_ptr<Structure> strc, Transform & initTransform);
        RenderElement(Viewport * view, std::shared_ptr<Model> model, std::shared_ptr<Material> mat, int scSize, Transform & initTrasnsform, std::vector<Shader::Binding> binds);
        RenderElement(Viewport * view, std::shared_ptr<Structure> strc, Transform & initTransform, std::vector<Shader::Binding> binds);
        virtual ~RenderElement();

        glm::mat4 getTransformationMatrix(Transform & instance);

        virtual void render(VkCommandBuffer & cmdBuffer, uint32_t frameIndex);
        virtual void renderShaderless(VkCommandBuffer & buffer, uint32_t frameIndex);

        Instance addInstance(Transform & trans);
        void updateInstance(Instance & instance, Transform & trans);
        void deleteInstance(Instance & instance);

        virtual void createUniformBuffers(int scSize, std::vector<Shader::Binding> & bindings);
        virtual void destroyUniformBuffers(const vkutil::SwapChain & swapchain);

        void recreateResources(VkRenderPass & renderPass, int scSize, const vkutil::SwapChain & swapchain);

        virtual void updateUniformBuffer(UniformBufferObject & obj, uint32_t frameIndex);

        std::vector<VkDescriptorSet> & getDescriptorSets();
        std::vector<VmaAllocation> & getMemories();

        bool needsDrawCmdUpdate();

        void recordTransfer(VkCommandBuffer & cmdBuffer);
        bool reusable();

        Shader * getShader();

        static RenderElement * buildRenderElement(Viewport * view, std::shared_ptr<Structure> strc, Transform & initTransform);
        static RenderElement * buildRenderElement(Viewport * view, std::shared_ptr<Model> model, std::shared_ptr<Material> material, Transform & initTransform);

    protected:

        static glm::mat4 toGLMMatrx(Math::Matrix<4,4,float> mat);

        const vkutil::VulkanState & state;

        VkDescriptorSetLayout descSetLayout;
        std::vector<VkDescriptorSet> descriptorSets;
        std::vector<VkBuffer> uniformBuffers;
        std::vector<VmaAllocation> uniformBuffersMemory;

        std::vector<Shader::Binding> binds;

        bool instanceBufferDirty;
        bool instanceCountUpdated;

    private:

        struct InstanceInfo {

            uint32_t id;
            uint32_t pos;

        };

        void constructBuffers(int scSize);

        std::shared_ptr<Model> model;
        std::shared_ptr<Shader> shader;
        std::vector<std::shared_ptr<Texture>> texture;

        VkDescriptorPool descPool;
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;

        DynamicBuffer<glm::mat4> * instanceBuffer;
        std::vector<glm::mat4> instanceTransforms;
        std::unordered_map<uint32_t, InstanceInfo> instances;
        std::vector<Transform> transforms;
        uint32_t instanceCount;

        std::mutex transformBufferMutex;

        RenderElement(Viewport * view, std::shared_ptr<Model> model, std::shared_ptr<Shader> shader, std::vector<std::shared_ptr<Texture>> texture, int scSize, Transform & initTransform);
        void markBufferDirty();


};

#endif // RENDERELEMENT_H
