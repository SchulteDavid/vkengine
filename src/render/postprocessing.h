#ifndef POSTPROCESSING_H
#define POSTPROCESSING_H

#include <memory>

#include "render/shader.h"

class PPEffect {

 public:
  PPEffect(std::shared_ptr<Shader> shader);
  virtual ~PPEffect();

  void setupPipeline(const vkutil::VulkanState & state, const vkutil::SwapChain & swapchain, VkRenderPass renderPass, uint32_t subpassId);
  void destroyPipeline();

  void bindForRender(VkCommandBuffer & buffer);

 private:
  VkPipeline pipeline;
  VkPipelineLayout pipelineLayout;

  VkDescriptorSetLayout descriptorLayout;
  
  std::shared_ptr<Shader> shader;
  
};

#endif
