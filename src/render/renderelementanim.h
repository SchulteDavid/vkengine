#ifndef RENDERELEMENTANIM_H
#define RENDERELEMENTANIM_H

#include "renderelement.h"
#include "animation/skeletalrig.h"

class RenderElementAnim : public RenderElement {

public:
  RenderElementAnim(Viewport * view, std::shared_ptr<Structure> strc, Transform<float> & initTransform);
  virtual ~RenderElementAnim();
  
  void createUniformBuffers(int scSize, std::vector<Shader::Binding> & bindings) override;
  virtual void destroyUniformBuffers(const vkutil::SwapChain & swapchain);
  
  virtual void updateUniformBuffer(UniformBufferObject & obj, uint32_t frameIndex);
  
  void setSkin(std::shared_ptr<Skin> skin);
  
protected:
  
private:
  
  std::vector<VkBuffer> animationBuffers;
  std::vector<VmaAllocation> animationBuffersMemory;
  
  std::vector<Shader::Binding> getShaderBindings(std::shared_ptr<Material> material);
  std::shared_ptr<Skin> skin;
  
};

#endif // RENDERELEMENTANIM_H
