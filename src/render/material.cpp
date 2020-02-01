#include "material.h"

#include "model.h"

#include <configloading.h>

#include <iostream>

Material::Material(std::shared_ptr<Shader> shader, std::vector<std::shared_ptr<Texture>> textures) {

    this->shader = shader;
    this->textures = textures;

}

Material::~Material() {



}

std::shared_ptr<Shader> Material::getShader() {
    return shader;
}

std::vector<std::shared_ptr<Texture>> Material::getTextures() {
    return textures;
}

void Material::prepareDescriptors() {

    VkSampler sampler = textures[0]->getSampler();
    shader->setupDescriptorSetLayout(sampler, textures.size());

}

VkPipeline Material::setupPipeline(const vkutil::VulkanState & state, const VkRenderPass & renderPass, const VkExtent2D & swapChainExtent, Model * model, VkPipelineLayout & layout) {

    vkutil::VertexInputDescriptions descs;
    descs.binding = model->getBindingDescription();
    descs.attributes = model->getAttributeDescriptions();

    return shader->setupGraphicsPipeline(descs, renderPass, state, swapChainExtent, layout);

}

MaterialUploader::MaterialUploader(const vkutil::VulkanState & state, const VkRenderPass & renderPass, const VkExtent2D & swapChainExtent, LoadingResource shader, std::vector<LoadingResource> textures) : state(state), renderPass(renderPass), swapChainExtent(swapChainExtent) {

    this->shader = shader;
    this->textures = textures;

}

Material * MaterialUploader::uploadResource() {

    std::shared_ptr<Shader> mShader = std::dynamic_pointer_cast<Shader>(shader->location);
    std::vector<std::shared_ptr<Texture>> mTextures(textures.size());

    for (unsigned int i = 0; i < textures.size(); ++i) {

        mTextures[i] = std::dynamic_pointer_cast<Texture>(textures[i]->location);

    }

    Material * mat = new Material(mShader, mTextures);

    //mat->setupPipeline(state, renderPass, swapChainExtent);
    mat->prepareDescriptors();

    return mat;

}

bool MaterialUploader::uploadReady() {

    bool texturesOk = true;

    for (LoadingResource r : textures) {
        texturesOk &= r->status.isUseable;
    }

    return shader->status.isUseable && texturesOk;

}

MaterialLoader::MaterialLoader(const vkutil::VulkanState & state, const VkRenderPass & renderPass, const VkExtent2D & swapChainExtent) : state(state), renderPass(renderPass), swapChainExtent(swapChainExtent) {

}

std::shared_ptr<ResourceUploader<Material>> MaterialLoader::loadResource(std::string fname) {

    //CompoundNode * root = ConfigLoader::loadFileTree(fname);
    std::shared_ptr<NodeCompound> root = config::parseFile(fname);

    std::string shaderFname(root->getNode<char>("shader")->getRawData().get());
    //std::cout << "Pushing " << shaderFname << " on the queue" << std::endl;
    LoadingResource shaderRes = this->loadDependency("Shader", shaderFname);

    //std::cout << "Dependency queued" << std::endl;

    std::shared_ptr<Node<std::shared_ptr<NodeCompound>>> textureComps = root->getNode<std::shared_ptr<NodeCompound>>("textures");
    std::cout << "Texture Array is ok" << std::endl;
    std::vector<LoadingResource> textureRes(textureComps->getElementCount());

    std::cout << "Loading textures " << std::endl;

    for (unsigned int i = 0; i < textureComps->getElementCount(); ++i) {

        std::shared_ptr<NodeCompound> texComp = textureComps->getElement(i);

        std::string tFname(texComp->getNode<char>("fname")->getRawData().get());
        int index = texComp->getNode<int>("index")->getElement(0);

        textureRes[index] = this->loadDependency("Texture", tFname);

    }

    return std::shared_ptr<MaterialUploader>(new MaterialUploader(state, renderPass, swapChainExtent, shaderRes, textureRes));

}

