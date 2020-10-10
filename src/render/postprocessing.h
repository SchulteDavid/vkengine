#ifndef POSTPROCESSING_H
#define POSTPROCESSING_H

#include <memory>

#include "render/shader.h"

class PPEffect {

 public:
  PPEffect(std::shared_ptr<Shader> shader);
  virtual ~PPEffect();

  void setupPipeline(const vkutil::VulkanState & state, const vkutil::SwapChain & swapchain, VkRenderPass renderPass, uint32_t subpassId);
  void createDescriptorSetLayout(const vkutil::VulkanState & state);
  void createDescriptorPool(const vkutil::VulkanState & state, const vkutil::SwapChain & swapchain);
  void createDescriptorSet(const vkutil::VulkanState & state, const vkutil::SwapChain & swapchain, VkImageView & inputImageView, VkImageView & depthView);
  void destroyPipeline();

  void bindForRender(VkCommandBuffer & buffer);
  void bindDescriptorSets(VkCommandBuffer & buffer, uint32_t frameIndex);

 private:
  VkPipeline pipeline;
  VkPipelineLayout pipelineLayout;

  VkDescriptorSetLayout descriptorLayout;

  VkDescriptorPool descriptorPool;
  std::vector<VkDescriptorSet> descriptorSets;
  
  std::shared_ptr<Shader> shader;
  
};

#endif
