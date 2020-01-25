#include "material.h"

#include "model.h"

#include <configloader.h>

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

void Material::setupPipeline(const vkutil::VulkanState & state, const VkRenderPass & renderPass, const VkExtent2D & swapChainExtent) {

    VkSampler sampler = textures[0]->getSampler();

    shader->setupDescriptorSetLayout(sampler, textures.size());

    vkutil::VertexInputDescriptions descs;
    descs.binding = Model::Vertex::getBindingDescription();
    descs.attributes = Model::Vertex::getAttributeDescriptions();

    shader->setupGraphicsPipeline(descs, renderPass, state, swapChainExtent);

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

    mat->setupPipeline(state, renderPass, swapChainExtent);

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

    CompoundNode * root = ConfigLoader::loadFileTree(fname);

    std::string shaderFname(root->get<const char *>("shader"));
    //std::cout << "Pushing " << shaderFname << " on the queue" << std::endl;
    LoadingResource shaderRes = this->loadDependency("Shader", shaderFname);

    //std::cout << "Dependency queued" << std::endl;

    std::vector<CompoundNode *> textureComps = root->getArray<CompoundNode *>("textures")->getValue();
    std::cout << "Texture Array is ok" << std::endl;
    std::vector<LoadingResource> textureRes(textureComps.size());

    std::cout << "Loading textures " << std::endl;

    for (CompoundNode * texComp : textureComps) {

        std::string tFname(texComp->get<const char *>("fname"));
        int index = texComp->get<int>("index");

        textureRes[index] = this->loadDependency("Texture", tFname);

    }

    return std::shared_ptr<MaterialUploader>(new MaterialUploader(state, renderPass, swapChainExtent, shaderRes, textureRes));

}

