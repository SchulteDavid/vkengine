#ifndef SHADER_H
#define SHADER_H

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

#include <memory>

#include "util/vkutil.h"
#include "texture.h"
#include "../resources/resource.h"

class Shader : public Resource
{
    public:
        Shader(std::string vertShader, std::string fragShader, const VkDevice & device);
        virtual ~Shader();

        std::vector<VkDescriptorSetLayoutBinding> getBindings(VkSampler & sampler, unsigned int textureCount);

        void setupDescriptorSetLayout(VkSampler & sampler, unsigned int texCount);
        void setupGraphicsPipeline(vkutil::VertexInputDescriptions & descs, const VkRenderPass & renderPass, const vkutil::VulkanState & state, VkExtent2D swapChainExtent);

        VkDescriptorPool setupDescriptorPool(const VkDevice & device, int scSize, int textureCount);
        std::vector<VkDescriptorSet> createDescriptorSets(const VkDevice & device, VkDescriptorPool & descPool, std::vector<VkBuffer> & uniformBuffers, size_t elementSize, std::vector<std::shared_ptr<Texture>>& tex, int scSize); /// <- To be stored in RenderElement
        VkPipeline & getPipeline();
        VkPipelineLayout & getPipelineLayout();

        void bindForRender(VkCommandBuffer & cmdBuffer, VkDescriptorSet & descriptors);

    protected:

    private:

        const VkDevice & device;

        VkPipeline graphicsPipeline;
        VkPipelineLayout pipelineLayout;
        VkDescriptorSetLayout descSetLayout;

        std::vector<VkShaderModule> modules;
        std::vector<VkShaderStageFlagBits> stages;
        std::vector<VkDescriptorSet> descSets;
        bool hasDescSets;

};

#endif // SHADER_H
