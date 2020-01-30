#include "shader.h"

#include <iostream>
#include <configloader.h>

#include "model.h"

#include "util/debug/trace_exception.h"

Shader::Shader(std::string vertShader, std::string fragShader, const VkDevice & device) : device(device) {

    this->modules.resize(2);
    this->stages = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

    vertShaderCode = readFile(vertShader);
    fragShaderCode = readFile(fragShader);

    modules[0] = vkutil::createShaderModule(vertShaderCode, device);
    modules[1] = vkutil::createShaderModule(fragShaderCode, device);

    hasDescSets = false;

}

Shader::Shader(std::vector<uint8_t> vertCode, std::vector<uint8_t> fragCode, const vkutil::VulkanState & state) : device(state.device) {

    vertShaderCode = vertCode;
    fragShaderCode = fragCode;

    hasDescSets = false;

}

Shader::~Shader() {

    for (unsigned int i = 0; i < modules.size(); ++i)
        vkDestroyShaderModule(device, modules[i], nullptr);

}

void Shader::createModules(const vkutil::VulkanState & state) {

    modules.resize(2);
    stages = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

    modules[0] = vkutil::createShaderModule(vertShaderCode, state.device);
    modules[1] = vkutil::createShaderModule(fragShaderCode, state.device);

}

std::vector<VkDescriptorSetLayoutBinding> Shader::getBindings(VkSampler & sampler, unsigned int textureCount) {

    std::vector<VkDescriptorSetLayoutBinding> bindings(1 + textureCount);

    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    bindings[0] = uboLayoutBinding;

    for (unsigned int i = 0; i < textureCount; ++i) {

        VkDescriptorSetLayoutBinding texLayoutBinding = {};
        texLayoutBinding.binding = i+1;
        texLayoutBinding.descriptorCount = 1;
        texLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        texLayoutBinding.pImmutableSamplers = &sampler;
        texLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[i + 1] = texLayoutBinding;

    }

    return bindings;

}

void Shader::setupDescriptorSetLayout(VkSampler & sampler, unsigned int textures) {

    std::vector<VkDescriptorSetLayoutBinding> bindings = getBindings(sampler, textures);
    descSetLayout = vkutil::createDescriptorSetLayout(bindings, device);

}

void Shader::bindForRender(VkCommandBuffer & cmdBuffer, VkDescriptorSet & descriptors) {

    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptors, 0, nullptr);

}

VkPipeline Shader::setupGraphicsPipeline(vkutil::VertexInputDescriptions & descs, const VkRenderPass & renderPass, const vkutil::VulkanState & state, VkExtent2D swapChainExtent, VkPipelineLayout & pipelineLayout) {

    std::vector<vkutil::ShaderInputDescription> inputShaders(modules.size());
    for (unsigned int i = 0; i < modules.size(); ++i) {

        inputShaders[i].entryName = "main";
        inputShaders[i].module = modules[i];
        inputShaders[i].usage = stages[i];

    }

    VkPipeline graphicsPipeline = vkutil::createGraphicsPipeline(state, renderPass, inputShaders, descs, descSetLayout, pipelineLayout, swapChainExtent);

    return graphicsPipeline;

}

VkDescriptorPool Shader::setupDescriptorPool(const VkDevice& device, int scSize, int textureCount) {

    VkDescriptorPoolSize poolSize = {};
    poolSize.type =  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = scSize;

    VkDescriptorPoolSize samplerSize = {};
    samplerSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerSize.descriptorCount = scSize;

    //VkDescriptorPoolSize sizes[] = {poolSize, samplerSize, samplerSize};
    std::vector<VkDescriptorPoolSize> sizes(1 + textureCount);
    sizes[0] = poolSize;
    for (int i = 1; i < textureCount+1; ++i)
        sizes[i] = samplerSize;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1 + textureCount;
    poolInfo.pPoolSizes = sizes.data();
    poolInfo.maxSets = scSize;

    VkDescriptorPool descriptorPool;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
        throw dbg::trace_exception("Unable to create descriptor pool");

    return descriptorPool;

}

std::vector<VkDescriptorSet> Shader::createDescriptorSets(const VkDevice & device, VkDescriptorPool & descPool, std::vector<VkBuffer> & uniformBuffers, size_t elementSize, std::vector<std::shared_ptr<Texture>> & tex, int scSize) {

    if (descPool == VK_NULL_HANDLE)
        throw dbg::trace_exception("Cannot create descriptor in NULL-pool!");

    std::cout << "Creating descriptor set with " << tex.size() << " textures" << std::endl;

    std::vector<VkDescriptorSetLayout> layouts(scSize, descSetLayout);

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descPool;
    allocInfo.descriptorSetCount = scSize;
    allocInfo.pSetLayouts = layouts.data();

    this->descSets.resize(scSize);
    if (vkAllocateDescriptorSets(device, &allocInfo, descSets.data()) != VK_SUCCESS)
        throw dbg::trace_exception("Unable to allocate descriptor sets");

    for (int i = 0; i < scSize; ++i) {

        int index = 0;

        std::vector<VkWriteDescriptorSet> descriptorWrites(1 + tex.size());

            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = elementSize;

            descriptorWrites[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[index].dstSet = descSets[i];
            descriptorWrites[index].dstBinding = 0;
            descriptorWrites[index].dstArrayElement = 0;
            descriptorWrites[index].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[index].descriptorCount = 1;
            descriptorWrites[index].pBufferInfo = &bufferInfo;
            descriptorWrites[index].pImageInfo = nullptr;
            descriptorWrites[index].pTexelBufferView = nullptr;
            index++;

        std::vector<VkDescriptorImageInfo> imageInfos(tex.size());

        for (unsigned int j = 0; j < tex.size(); ++j) {

            imageInfos[j].imageView = tex[j]->getView();
            imageInfos[j].sampler = tex[j]->getSampler();
            imageInfos[j].imageLayout = tex[j]->getLayout();


            descriptorWrites[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[index].dstSet = descSets[i];
            descriptorWrites[index].dstBinding = index;
            descriptorWrites[index].dstArrayElement = 0;
            descriptorWrites[index].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[index].descriptorCount = 1;
            descriptorWrites[index].pBufferInfo = nullptr;
            descriptorWrites[index].pImageInfo = &imageInfos[j];
            descriptorWrites[index].pTexelBufferView = nullptr;

            index++;

        }

        vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

    }

    return descSets;

}

VkPipeline & Shader::getPipeline() {
    return graphicsPipeline;
}

VkPipelineLayout & Shader::getPipelineLayout() {
    return this->pipelineLayout;
}

ShaderLoader::ShaderLoader(const vkutil::VulkanState & state) : state(state) {

}

std::shared_ptr<ResourceUploader<Shader>> ShaderLoader::loadResource(std::string fname) {

    CompoundNode * root = ConfigLoader::loadFileTree(fname);

    std::string vertFname(root->get<const char *>("vertex"));
    std::string fragFname(root->get<const char *>("fragment"));


    //std::cout << vertFname << "  " << fragFname << std::endl;

    //delete root;

    std::vector<uint8_t> vertCode = readFile(vertFname);
    std::vector<uint8_t> fragCode = readFile(fragFname);

    Shader * shader = new Shader(vertCode, fragCode, state);

    return std::shared_ptr<ShaderUploader>(new ShaderUploader(state, shader));

}

ShaderUploader::ShaderUploader(const vkutil::VulkanState & state, Shader * shader) : state(state) {

    this->shader = shader;

}

Shader * ShaderUploader::uploadResource() {

    this->shader->createModules(state);
    return shader;

}

bool ShaderUploader::uploadReady() {
    return true;
}

