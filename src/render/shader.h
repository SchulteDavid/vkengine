#ifndef SHADER_H
#define SHADER_H

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

#include <memory>

#include "render/util/vkutil.h"
#include "texture.h"
#include "resources/resource.h"
#include "resources/resourceloader.h"
#include "resources/resourceuploader.h"

class Shader : public Resource
{
    public:
        Shader(std::vector<uint8_t> vertCode, std::vector<uint8_t> fragCode, const vkutil::VulkanState & state, unsigned int textureSlots);
        virtual ~Shader();

        std::vector<VkDescriptorSetLayoutBinding> getBindings(VkSampler & sampler);

        void setupDescriptorSetLayout(VkSampler & sampler);
        VkPipeline setupGraphicsPipeline(vkutil::VertexInputDescriptions & descs, const VkRenderPass & renderPass, const vkutil::VulkanState & state, VkExtent2D swapChainExtent, VkPipelineLayout & pipelineLayout);

        VkDescriptorPool setupDescriptorPool(const VkDevice & device, int scSize);
        std::vector<VkDescriptorSet> createDescriptorSets(const VkDevice & device, VkDescriptorPool & descPool, std::vector<VkBuffer> & uniformBuffers, size_t elementSize, std::vector<std::shared_ptr<Texture>>& tex, int scSize); /// <- To be stored in RenderElement
        VkPipeline & getPipeline();
        VkPipelineLayout & getPipelineLayout();

        void bindForRender(VkCommandBuffer & cmdBuffer, VkDescriptorSet & descriptors);

        void createModules(const vkutil::VulkanState & state);

    protected:

        Shader(std::string vertShader, std::string fragShader, const VkDevice & device);

    private:

        const VkDevice & device;
        unsigned int textureSlots;

        VkPipeline graphicsPipeline;
        VkPipelineLayout pipelineLayout;
        VkDescriptorSetLayout descSetLayout;

        std::vector<VkShaderModule> modules;
        std::vector<VkShaderStageFlagBits> stages;
        std::vector<VkDescriptorSet> descSets;
        bool hasDescSets;

        std::vector<uint8_t> vertShaderCode;
        std::vector<uint8_t> fragShaderCode;

};

class ShaderUploader : public ResourceUploader<Shader> {

    public:

        ShaderUploader(const vkutil::VulkanState & state, Shader * shader);

        Shader * uploadResource();
        bool uploadReady();

    private:

        const vkutil::VulkanState & state;
        Shader * shader;

};

class ShaderLoader : public ResourceLoader<Shader> {

    public:

        ShaderLoader(const vkutil::VulkanState & state);

        std::shared_ptr<ResourceUploader<Shader>> loadResource(std::string fname);

    private:

        const vkutil::VulkanState & state;

};

#endif // SHADER_H
