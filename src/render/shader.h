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
#include "util/mesh.h"

class Shader : public Resource
{
    public:

        enum BindingType {

            BINDING_NONE,
            BINDING_UNIFORM_BUFFER,
            BINDING_TEXTURE_SAMPLER,

        };

        struct Binding {

            BindingType type;
            uint32_t bindingId;
            uint32_t elementCount; // The shader will automaticaly adjust this to be
                                   // the value specified in the resource file.

            std::vector<VkBuffer> uniformBuffers;
            size_t elementSize;

        };

        Shader(std::vector<uint8_t> vertCode, std::vector<uint8_t> fragCode, const vkutil::VulkanState & state, unsigned int textureSlots);
        virtual ~Shader();

        /**
            Will create the VkDescriptorSetLayoutBindings Bindings
        **/
        std::vector<VkDescriptorSetLayoutBinding> getVkBindings(std::vector<Binding> bindings);
        //std::vector<VkDescriptorSetLayoutBinding> getVkBindings(VkSampler & sampler);

        VkDescriptorSetLayout setupDescriptorSetLayout(std::vector<Shader::Binding> bindings);
        //void setupDescriptorSetLayout(VkSampler & sampler);
        VkPipeline setupGraphicsPipeline(vkutil::VertexInputDescriptions & descs, const VkRenderPass & renderPass, const vkutil::VulkanState & state, const VkDescriptorSetLayout & descLayout, VkExtent2D swapChainExtent, VkPipelineLayout & pipelineLayout);

        VkDescriptorPool setupDescriptorPool(int scSize, std::vector<Binding> & binds);
        //std::vector<VkDescriptorSet> createDescriptorSets(VkDescriptorPool & descPool, const VkDescriptorSetLayout & descLayout, std::vector<VkBuffer> & uniformBuffers, size_t elementSize, std::vector<std::shared_ptr<Texture>>& tex, int scSize); /// <- To be stored in RenderElement
        std::vector<VkDescriptorSet> createDescriptorSets(VkDescriptorPool & descPool, const VkDescriptorSetLayout & descLayout, std::vector<Binding> & binds, std::vector<std::shared_ptr<Texture>>& tex, int scSize);
        VkPipeline & getPipeline();
        VkPipelineLayout & getPipelineLayout();

        void bindForRender(VkCommandBuffer & cmdBuffer, VkDescriptorSet & descriptors);

        void createModules(const vkutil::VulkanState & state);

        const std::vector<InputDescription> & getInputs();
        void setInputs(std::vector<InputDescription> inputs);

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

        std::vector<InputDescription> inputs;
};

class ShaderUploader : public ResourceUploader<Shader> {

    public:

        /// TODO: complete migration to shared_ptr

        ShaderUploader(const vkutil::VulkanState & state, Shader * shader);

        std::shared_ptr<Shader> uploadResource();
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
