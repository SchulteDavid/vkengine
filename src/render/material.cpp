#include "material.h"

#include "model.h"

#include <configloading.h>
#include "render/renderelement.h"
#include <iostream>

Material::Material(std::shared_ptr<Shader> shader, std::vector<std::shared_ptr<Texture>> textures) {

    this->shader = shader;
    this->textures = textures;
    this->staticShader = nullptr;

}

Material::Material(std::shared_ptr<Shader> shader, std::shared_ptr<Shader> staticShader, std::vector<std::shared_ptr<Texture>> textures) : Material(shader, textures) {

  this->staticShader = staticShader;

}

Material::~Material() {



}

std::shared_ptr<Shader> Material::getShader() {
    return shader;
}

std::shared_ptr<Shader> Material::getStaticShader() {
    return staticShader;
}

std::vector<std::shared_ptr<Texture>> Material::getTextures() {
    return textures;
}

std::vector<Shader::Binding> Material::getDefaultBindings() {
  std::vector<Shader::Binding> binds;

  Shader::Binding uniformBufferBinding;
  uniformBufferBinding.type = Shader::BINDING_UNIFORM_BUFFER;
  uniformBufferBinding.elementCount = 1;
  uniformBufferBinding.bindingId = 0;
  uniformBufferBinding.elementSize = sizeof(UniformBufferObject);
  binds.push_back(uniformBufferBinding);

  std::cout << "Material has " << textures.size() << " textures" << std::endl;

  if (this->textures.size()) {
    Shader::Binding textureBinding;
    textureBinding.type = Shader::BINDING_TEXTURE_SAMPLER;
    textureBinding.bindingId = 1;
    binds.push_back(textureBinding);
  }

  return binds;
}

VkDescriptorSetLayout Material::prepareDescriptors(std::vector<Shader::Binding> bindings) {

    return shader->setupDescriptorSetLayout(bindings);

}

VkPipeline Material::setupPipeline(const vkutil::VulkanState & state, const VkRenderPass & renderPass, const VkExtent2D & swapChainExtent, const VkDescriptorSetLayout & descLayout, Model * model, VkPipelineLayout & layout) {

    vkutil::VertexInputDescriptions descs;
    descs.binding = model->getBindingDescription();
    descs.attributes = model->getAttributeDescriptions();

    return shader->setupGraphicsPipeline(descs, renderPass, state, descLayout, swapChainExtent, layout);

}

VkPipeline Material::setupStaticPipeline(const vkutil::VulkanState & state, const VkRenderPass & renderPass, const VkExtent2D & swapChainExtent, const VkDescriptorSetLayout & descLayout, Model * model, VkPipelineLayout & layout) {

    vkutil::VertexInputDescriptions descs;
    descs.binding = model->getBindingDescription();
    descs.attributes = model->getAttributeDescriptions();

    return staticShader->setupGraphicsPipeline(descs, renderPass, state, descLayout, swapChainExtent, layout);

}

MaterialUploader::MaterialUploader(const vkutil::VulkanState & state, const VkRenderPass & renderPass, const VkExtent2D & swapChainExtent, LoadingResource shader, std::vector<LoadingResource> textures) : state(state), renderPass(renderPass), swapChainExtent(swapChainExtent) {

    this->shader = shader;
    this->textures = textures;

}

MaterialUploader::MaterialUploader(const vkutil::VulkanState & state, const VkRenderPass & renderPass, const VkExtent2D & swapChainExtent, LoadingResource shader, LoadingResource staticShader, std::vector<LoadingResource> textures) : MaterialUploader(state, renderPass, swapChainExtent, shader, textures) {

    this->staticShader = staticShader;

}

std::shared_ptr<Material> MaterialUploader::uploadResource() {

    std::shared_ptr<Shader> mShader = std::dynamic_pointer_cast<Shader>(shader->location);

    std::vector<std::shared_ptr<Texture>> mTextures(textures.size());

    for (unsigned int i = 0; i < textures.size(); ++i) {

        mTextures[i] = std::dynamic_pointer_cast<Texture>(textures[i]->location);

    }

    Material * mat;
    if (!this->staticShader) {

      mat = new Material(mShader, mTextures);

    } else {

      std::shared_ptr<Shader> sShader = std::dynamic_pointer_cast<Shader>(staticShader->location);
      mat = new Material(mShader, sShader, mTextures);

    }

    //mat->setupPipeline(state, renderPass, swapChainExtent);
    //mat->prepareDescriptors();

    return std::shared_ptr<Material>(mat);

}

bool MaterialUploader::uploadReady() {

    bool texturesOk = true;

    for (LoadingResource r : textures) {

        texturesOk &= r->status.isUseable;
    }

    return shader->status.isUseable && (!staticShader || staticShader->status.isUseable) && texturesOk;

}

MaterialLoader::MaterialLoader(const vkutil::VulkanState & state, const VkRenderPass & renderPass, const VkExtent2D & swapChainExtent) : state(state), renderPass(renderPass), swapChainExtent(swapChainExtent) {

}

ResourceUploader<Material> * MaterialLoader::buildResource(std::shared_ptr<config::NodeCompound> root) {

  using namespace config;

  std::string shaderFname(root->getNode<char>("shader")->getRawData());
  std::cout << "Pushing " << shaderFname << " on the queue" << std::endl;
  LoadingResource shaderRes = this->loadDependency(ResourceLocation("Shader", shaderFname));

  LoadingResource staticShaderRes = nullptr;

  std::cout << "Dependency queued" << std::endl;

  if (root->hasChild("staticShader")) {

    std::string staticShaderName(root->getNode<char>("staticShader")->getRawData());
    staticShaderRes = this->loadDependency(ResourceLocation("Shader", staticShaderName));

  }

  if (root->hasChild("textures")) {

    std::shared_ptr<Node<std::shared_ptr<NodeCompound>>> textureComps = root->getNode<std::shared_ptr<NodeCompound>>("textures");
    std::cout << "Texture Array is ok" << std::endl;
    std::vector<LoadingResource> textureRes(textureComps->getElementCount());

    std::cout << "Loading textures " << std::endl;

    for (unsigned int i = 0; i < textureComps->getElementCount(); ++i) {

      std::shared_ptr<NodeCompound> texComp = textureComps->getElement(i);

      std::string tFname(texComp->getNode<char>("fname")->getRawData());
      int index = texComp->getNode<int>("index")->getElement(0);

      std::cout << tFname << " -> " << index << " / " << textureComps->getElementCount() << std::endl;

      textureRes[index] = this->loadDependency(ResourceLocation("Texture", tFname));

    }

    std::cout << "Textures Loaded" << std::endl;

    return new MaterialUploader(state, renderPass, swapChainExtent, shaderRes, staticShaderRes, textureRes);

  }

  return new MaterialUploader(state, renderPass, swapChainExtent, shaderRes, staticShaderRes, {});
}

std::shared_ptr<ResourceUploader<Material>> MaterialLoader::loadResource(std::string fname) {

    using namespace config;

    //CompoundNode * root = ConfigLoader::loadFileTree(fname);
    std::shared_ptr<NodeCompound> root = config::parseFile(fname);

    std::cout << "Loading material from compound " << root << std::endl;

    return std::shared_ptr<MaterialUploader>((MaterialUploader *)buildResource(root));

}

