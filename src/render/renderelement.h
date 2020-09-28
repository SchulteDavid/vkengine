#ifndef RENDERELEMENT_H
#define RENDERELEMENT_H

#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>

#include "model.h"
#include "texture.h"
#include "shader.h"
#include "dynamicbuffer.h"
#include <mathutils/quaternion.h>
#include "memorytransferhandler.h"
#include "material.h"
#include "structure/structure.h"

#include "util/transform.h"

class Viewport;

struct UniformBufferObject {

  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;

};


class RenderElement : public MemoryTransferer {

public:

  /*struct Transform {
    Math::Vector<4, float> position;
    Math::Quaternion<float> qRot;
    float scale;
    };*/

  struct Instance {
    uint32_t id;
  };
  
  RenderElement(Viewport * view, std::shared_ptr<Model> model, std::shared_ptr<Material> mat, int scSize, Transform<float> & initTransform);
  RenderElement(Viewport * view, std::shared_ptr<Structure> strc, Transform<float> & initTransform);
  RenderElement(Viewport * view, std::shared_ptr<Model> model, std::shared_ptr<Material> mat, int scSize, Transform<float> & initTransform, std::vector<Shader::Binding> binds);
  RenderElement(Viewport * view, std::shared_ptr<Structure> strc, Transform<float> & initTransform, std::vector<Shader::Binding> binds);
  virtual ~RenderElement();

  glm::mat4 getTransformationMatrixGLM(Transform<float> & instance);

  virtual void render(VkCommandBuffer & cmdBuffer, uint32_t frameIndex);
  virtual void renderShaderless(VkCommandBuffer & buffer, uint32_t frameIndex);

  virtual Instance addInstance(Transform<float> & trans);
  virtual void updateInstance(Instance & instance, Transform<float> & trans);
  virtual void deleteInstance(Instance & instance);

  virtual void createUniformBuffers(int scSize, std::vector<Shader::Binding> & bindings);
  virtual void destroyUniformBuffers(const vkutil::SwapChain & swapchain);

  void recreateResources(VkRenderPass & renderPass, int scSize, const vkutil::SwapChain & swapchain);

  virtual void updateUniformBuffer(UniformBufferObject & obj, uint32_t frameIndex);

  std::vector<VkDescriptorSet> & getDescriptorSets();
  std::vector<VmaAllocation> & getMemories();

  virtual bool needsDrawCmdUpdate();

  void recordTransfer(VkCommandBuffer & cmdBuffer);
  bool reusable();

  Shader * getShader();

  static RenderElement * buildRenderElement(Viewport * view, std::shared_ptr<Structure> strc, Transform<float> & initTransform);
  static RenderElement * buildRenderElement(Viewport * view, std::shared_ptr<Model> model, std::shared_ptr<Material> material, Transform<float> & initTransform);

protected:

  static glm::mat4 toGLMMatrx(Math::Matrix<4,4,float> mat);

  const vkutil::VulkanState & state;

  VkDescriptorSetLayout descSetLayout;
  std::vector<VkDescriptorSet> descriptorSets;
  std::vector<VkBuffer> uniformBuffers;
  std::vector<VmaAllocation> uniformBuffersMemory;

  std::vector<Shader::Binding> binds;
  Transform<float> transform;

  virtual void constructBuffers(int scSize);
  void markBufferDirty();

  std::shared_ptr<Model> model;
  std::shared_ptr<Shader> shader;
  std::vector<std::shared_ptr<Texture>> texture;

  VkDescriptorPool descPool;
  VkPipeline pipeline;
  VkPipelineLayout pipelineLayout;

private:

  RenderElement(Viewport * view, std::shared_ptr<Model> model, std::shared_ptr<Shader> shader, std::vector<std::shared_ptr<Texture>> texture, int scSize, Transform<float> & initTransform);


};

#endif // RENDERELEMENT_H
