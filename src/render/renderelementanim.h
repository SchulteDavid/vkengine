#ifndef RENDERELEMENTANIM_H
#define RENDERELEMENTANIM_H

#include "renderelement.h"
#include "animation/skeletalrig.h"

class RenderElementAnim : public RenderElement {

public:
  RenderElementAnim(Viewport * view, std::shared_ptr<Model> model, std::shared_ptr<Material> material, Transform<float> & initTransform, std::shared_ptr<Skin> skin);
  virtual ~RenderElementAnim();
  
  void createUniformBuffers(int scSize, std::vector<Shader::Binding> & bindings) override;
  virtual void destroyUniformBuffers(const vkutil::SwapChain & swapchain) override;
  
  virtual void updateUniformBuffer(UniformBufferObject & obj, uint32_t frameIndex) override;
  
  void setSkin(std::shared_ptr<Skin> skin);
  
protected:
  
private:
  
  std::vector<VkBuffer> animationBuffers;
  std::vector<VmaAllocation> animationBuffersMemory;
  
  std::vector<Shader::Binding> getShaderBindings(std::shared_ptr<Material> material, std::shared_ptr<Skin> skin);
  std::shared_ptr<Skin> skin;
  
};

#endif // RENDERELEMENTANIM_H
