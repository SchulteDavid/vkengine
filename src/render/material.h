#ifndef MATERIAL_H
#define MATERIAL_H

#include <configloading.h>

#include "shader.h"
#include "texture.h"
#include "model.h"

#include "resources/resourceuploader.h"
#include "resources/resourceloader.h"

class Material : public Resource {

    public:
        Material(std::shared_ptr<Shader> shader, std::vector<std::shared_ptr<Texture>> textures);
        virtual ~Material();

        std::shared_ptr<Shader> getShader();
        std::vector<std::shared_ptr<Texture>> getTextures();


        VkDescriptorSetLayout prepareDescriptors(std::vector<Shader::Binding> bindings);
        VkPipeline setupPipeline(const vkutil::VulkanState & state, const VkRenderPass & renderPass, const VkExtent2D & swapChainExtent, const VkDescriptorSetLayout & descLayout, Model * model, VkPipelineLayout & layout);

    protected:

    private:

        std::shared_ptr<Shader> shader;
        std::vector<std::shared_ptr<Texture>> textures;

        /*VkDescriptorPool descPool;

        std::vector<VkDescriptorSet> descriptorSets;
        std::vector<VkBuffer> uniformBuffers;
        std::vector<VmaAllocation> uniformBuffersMemory;*/

};

class MaterialUploader : public ResourceUploader<Material> {

    public:

        MaterialUploader(const vkutil::VulkanState & state, const VkRenderPass & renderPass, const VkExtent2D & swapChainExtent, LoadingResource shader, std::vector<LoadingResource> textures);
        Material * uploadResource();
        bool uploadReady();

    private:

        const vkutil::VulkanState & state;
        const VkRenderPass & renderPass;
        const VkExtent2D & swapChainExtent;
        LoadingResource shader;
        std::vector<LoadingResource> textures;

};

class MaterialLoader : public ResourceLoader<Material> {

    public:

        MaterialLoader(const vkutil::VulkanState & state, const VkRenderPass & renderPass, const VkExtent2D & swapChainExtent);
        std::shared_ptr<ResourceUploader<Material>> loadResource(std::string fname);
        ResourceUploader<Material> * buildResource(std::shared_ptr<config::NodeCompound> root);


    private:

        const vkutil::VulkanState & state;
        const VkRenderPass & renderPass;
        const VkExtent2D & swapChainExtent;


};

#endif // MATERIAL_H
