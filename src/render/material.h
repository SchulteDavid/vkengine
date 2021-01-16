#ifndef MATERIAL_H
#define MATERIAL_H

#include <configloading.h>

#include "shader.h"
#include "texture.h"
#include "model.h"

#include "resources/resourceuploader.h"
#include "resources/resourceloader.h"

enum MaterialUsecase {
		      MAT_USE_DEFAULT,
		      MAT_USE_STATIC,
		      MAT_USE_SKIN,
};

class Material : public Resource {

public:
  Material(std::shared_ptr<Shader> shader, std::vector<std::shared_ptr<Texture>> textures);
  Material(std::shared_ptr<Shader> shader, std::shared_ptr<Shader> staticShader, std::vector<std::shared_ptr<Texture>> textures);
  Material(std::shared_ptr<Shader> shader, std::shared_ptr<Shader> staticShader, std::shared_ptr<Shader> skinShader, std::vector<std::shared_ptr<Texture>> textures);
  virtual ~Material();

  std::shared_ptr<Shader> getShader();
  std::shared_ptr<Shader> getStaticShader();
  std::shared_ptr<Shader> getSkinShader();

  std::shared_ptr<Shader> getShader(MaterialUsecase use);
  
  std::vector<std::shared_ptr<Texture>> getTextures();

  std::vector<Shader::Binding> getDefaultBindings();


  VkDescriptorSetLayout prepareDescriptors(std::vector<Shader::Binding> bindings);
  VkDescriptorSetLayout prepareDescriptors(std::vector<Shader::Binding> bindings, MaterialUsecase use);
  VkPipeline setupPipeline(const vkutil::VulkanState & state, const VkRenderPass & renderPass, const VkExtent2D & swapChainExtent, const VkDescriptorSetLayout & descLayout, Model * model, VkPipelineLayout & layout);
  VkPipeline setupStaticPipeline(const vkutil::VulkanState & state, const VkRenderPass & renderPass, const VkExtent2D & swapChainExtent, const VkDescriptorSetLayout & descLayout, Model * model, VkPipelineLayout & layout);

  VkPipeline setupPipeline(const vkutil::VulkanState & state, const VkRenderPass & renderPass, const VkExtent2D & swapChainExtent, const VkDescriptorSetLayout & descLayout, Model * model, VkPipelineLayout & layout, MaterialUsecase use);

protected:

private:

  std::shared_ptr<Shader> shader;
  std::shared_ptr<Shader> staticShader;
  std::shared_ptr<Shader> skinShader;
  std::vector<std::shared_ptr<Texture>> textures;

  /*VkDescriptorPool descPool;

    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VmaAllocation> uniformBuffersMemory;*/

};

class MaterialUploader : public ResourceUploader<Material> {

public:

  MaterialUploader(LoadingResource shader, std::vector<LoadingResource> textures);
  MaterialUploader(LoadingResource shader, LoadingResource staticShader, std::vector<LoadingResource> textures);
  MaterialUploader(LoadingResource shader, LoadingResource staticShader, LoadingResource skinShader, std::vector<LoadingResource> textures);
  std::shared_ptr<Material> uploadResource(vkutil::VulkanState & state, ResourceManager * manager);
  bool uploadReady();

private:

  //const vkutil::VulkanState & state;
  //const VkRenderPass & renderPass;
  //const VkExtent2D & swapChainExtent;
  LoadingResource shader;
  LoadingResource staticShader;
  LoadingResource skinShader;
  std::vector<LoadingResource> textures;

};

class MaterialLoader : public ResourceLoader<Material> {

public:

  MaterialLoader();
  std::shared_ptr<ResourceUploader<Material>> loadResource(std::string fname);
  ResourceUploader<Material> * buildResource(std::shared_ptr<config::NodeCompound> root);


private:

  /*const vkutil::VulkanState & state;
  const VkRenderPass & renderPass;
  const VkExtent2D & swapChainExtent;*/


};

#endif // MATERIAL_H
