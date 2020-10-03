#include "renderelement.h"

#include "storagebuffer.h"

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <string.h>
#include "viewport.h"

#include "renderelementanim.h"

RenderElement::RenderElement(Viewport * view, std::shared_ptr<Model> model, std::shared_ptr<Shader> shader, std::vector<std::shared_ptr<Texture>> texture, int scSize, Transform<float> & initTransform) : state(view->getState()), MemoryTransferer(*view) {

  std::vector<Shader::Binding> binds;

  Shader::Binding uniformBufferBinding;
  uniformBufferBinding.type = Shader::BINDING_UNIFORM_BUFFER;
  uniformBufferBinding.elementCount = 1;
  uniformBufferBinding.bindingId = 0;
  uniformBufferBinding.elementSize = sizeof(UniformBufferObject);
  binds.push_back(uniformBufferBinding);

  if (texture.size()) {
    Shader::Binding textureBinding;
    textureBinding.type = Shader::BINDING_TEXTURE_SAMPLER;
    textureBinding.bindingId = 1;
    binds.push_back(textureBinding);
  }

  this->binds = binds;

  this->model = model;
  this->shader = shader;
  this->texture = texture;

  std::array<float, 3> rAxis = {0.0, 0.0, 1.0};

  transform = initTransform;

}

RenderElement::RenderElement(Viewport * view, std::shared_ptr<Model> model, std::shared_ptr<Material> mat, int scSize, Transform<float> & initTransform) : state(view->getState()), MemoryTransferer(*view) {

  std::vector<Shader::Binding> binds;

  Shader::Binding uniformBufferBinding;
  uniformBufferBinding.type = Shader::BINDING_UNIFORM_BUFFER;
  uniformBufferBinding.elementCount = 1;
  uniformBufferBinding.bindingId = 0;
  uniformBufferBinding.elementSize = sizeof(UniformBufferObject);
  binds.push_back(uniformBufferBinding);

  if (mat->getTextures().size()) {
    Shader::Binding textureBinding;
    textureBinding.type = Shader::BINDING_TEXTURE_SAMPLER;
    textureBinding.bindingId = 1;
    binds.push_back(textureBinding);
  }


  this->binds = binds;

  this->model = model;
  this->shader = mat->getShader();
  this->texture = mat->getTextures();

  transform = initTransform;

  model->uploadToGPU(state.device, state.transferCommandPool, state.transferQueue);

  descSetLayout = mat->prepareDescriptors(this->binds);
  pipeline = mat->setupPipeline(state, view->getRenderpass(), view->getSwapchainExtent(), descSetLayout, model.get(), pipelineLayout);

}

RenderElement::RenderElement(Viewport * view, std::shared_ptr<Model> model, std::shared_ptr<Material> mat, int scSize, Transform<float> & initTransform, bool useStaticShader) : state(view->getState()), MemoryTransferer(*view) {

  std::vector<Shader::Binding> binds;

  Shader::Binding uniformBufferBinding;
  uniformBufferBinding.type = Shader::BINDING_UNIFORM_BUFFER;
  uniformBufferBinding.elementCount = 1;
  uniformBufferBinding.bindingId = 0;
  uniformBufferBinding.elementSize = sizeof(UniformBufferObject);
  binds.push_back(uniformBufferBinding);

  if (mat->getTextures().size()) {
    Shader::Binding textureBinding;
    textureBinding.type = Shader::BINDING_TEXTURE_SAMPLER;
    textureBinding.bindingId = 1;
    binds.push_back(textureBinding);
  }


  this->binds = binds;

  this->model = model;
  if (!useStaticShader) {
    this->shader = mat->getShader();
  } else {
    this->shader = mat->getStaticShader();
    if (!this->shader) {
      throw dbg::trace_exception("Used non-static material for static renderelement");
    }
  }
  this->texture = mat->getTextures();

  transform = initTransform;

  model->uploadToGPU(state.device, state.transferCommandPool, state.transferQueue);

  descSetLayout = mat->prepareDescriptors(this->binds);
  if (!useStaticShader) {
    pipeline = mat->setupPipeline(state, view->getRenderpass(), view->getSwapchainExtent(), descSetLayout, model.get(), pipelineLayout);
  } else {
    pipeline = mat->setupStaticPipeline(state, view->getRenderpass(), view->getSwapchainExtent(), descSetLayout, model.get(), pipelineLayout);
  }

}

RenderElement::RenderElement(Viewport * view, std::shared_ptr<Model> model, std::shared_ptr<Material> mat, int scSize, Transform<float> & initTransform, std::vector<Shader::Binding> binds) : state(view->getState()), MemoryTransferer(*view) {

  this->binds = binds;

  this->model = model;
  this->shader = mat->getShader();
  this->texture = mat->getTextures();

  model->uploadToGPU(state.device, state.transferCommandPool, state.transferQueue);

  descSetLayout = mat->prepareDescriptors(this->binds);
  pipeline = mat->setupPipeline(state, view->getRenderpass(), view->getSwapchainExtent(), descSetLayout, model.get(), pipelineLayout);


}

RenderElement::~RenderElement() {

  vkDestroyDescriptorPool(state.device, descPool, nullptr);

}

void RenderElement::constructBuffers(int scSize) {

  this->createUniformBuffers(scSize, this->binds);

  this->descPool = this->shader->setupDescriptorPool(scSize, binds);
  this->descriptorSets = shader->createDescriptorSets(descPool, descSetLayout, this->binds, texture, scSize);


}

glm::mat4 RenderElement::toGLMMatrx(Math::Matrix<4,4,float> m) {

  glm::mat4 mat = glm::mat4(1.0);
  mat[0] = glm::vec4(m(0,0), m(1,0), m(2,0), m(3,0));
  mat[1] = glm::vec4(m(0,1), m(1,1), m(2,1), m(3,1));
  mat[2] = glm::vec4(m(0,2), m(1,2), m(2,2), m(3,2));
  mat[3] = glm::vec4(m(0,3), m(1,3), m(2,3), m(3,3));

  return mat;

}

glm::mat4 RenderElement::getTransformationMatrixGLM(Transform<float> & i) {

  return toGLMMatrx(getTransformationMatrix(i));

}

void RenderElement::markBufferDirty() {
  this->handler.signalTransfer(this);
}

RenderElement::Instance RenderElement::addInstance(Transform<float> & i) {

  return (Instance) {0};

}

void RenderElement::updateInstance(Instance & instance, Transform<float> & trans) {

  transform = trans;

}

void RenderElement::deleteInstance(Instance & instance) {



}

void RenderElement::createUniformBuffers(int swapChainSize, std::vector<Shader::Binding> & bindings) {

  VkDeviceSize bufferSize = sizeof(UniformBufferObject);

  uniformBuffers.resize(swapChainSize);
  uniformBuffersMemory.resize(swapChainSize);

  for (int i = 0; i < swapChainSize; ++i) {

    VkBufferCreateInfo stBufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    stBufferCreateInfo.size = bufferSize;
    stBufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    stBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo stAllocCreateInfo = {};
    stAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    stAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo stagingBufferAllocInfo = {};

    vmaCreateBuffer(state.vmaAllocator, &stBufferCreateInfo, &stAllocCreateInfo, &uniformBuffers[i], &uniformBuffersMemory[i], &stagingBufferAllocInfo);

  }

  bindings[0].uniformBuffers = uniformBuffers;

}

void RenderElement::recreateResources(VkRenderPass & renderPass, int scSize, const vkutil::SwapChain & swapchain) {

  vkutil::VertexInputDescriptions descs;
  descs.attributes = this->model->getAttributeDescriptions();
  descs.binding = this->model->getBindingDescription();

  pipeline = shader->setupGraphicsPipeline(descs, renderPass, state, descSetLayout, swapchain.extent, pipelineLayout);

  descPool = shader->setupDescriptorPool(scSize, binds);
  createUniformBuffers(scSize, this->binds);
  this->descriptorSets = shader->createDescriptorSets(descPool, descSetLayout, this->binds, texture, scSize);

}

void RenderElement::destroyUniformBuffers(const vkutil::SwapChain & swapchain) {

  for (int i = 0; i < swapchain.images.size(); ++i) {

    vmaDestroyBuffer(state.vmaAllocator, uniformBuffers[i], uniformBuffersMemory[i]);

  }

}

void RenderElement::recordTransfer(VkCommandBuffer & cmdBuffer) {



}

bool RenderElement::reusable() {
  return true;
}

void RenderElement::updateUniformBuffer(UniformBufferObject & obj,  uint32_t imageIndex) {

  void * data;
  vmaMapMemory(state.vmaAllocator, uniformBuffersMemory[imageIndex], &data);
  memcpy(data, &obj, sizeof(UniformBufferObject));
  vmaUnmapMemory(state.vmaAllocator, uniformBuffersMemory[imageIndex]);

}

bool RenderElement::needsDrawCmdUpdate() {
  return false;
}

Shader * RenderElement::getShader() {
  return this->shader.get();
}

void RenderElement::render(VkCommandBuffer & cmdBuffer, uint32_t frameIndex) {

  //lout << "Pipeline " << pipeline << std::endl;
  vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[frameIndex], 0, nullptr);

  glm::mat4 data = toGLMMatrx(getTransformationMatrix(transform));

  vkCmdPushConstants(cmdBuffer, shader->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), &data);

  model->bindForRender(cmdBuffer);
  VkDeviceSize offsets[] = {0};
  //vkCmdBindVertexBuffers(cmdBuffer, 1, 1, &instanceBuffer->getBuffer(), offsets);

  vkCmdDrawIndexed(cmdBuffer, model->getIndexCount(), 1, 0, 0, 0);

}

void RenderElement::renderShaderless(VkCommandBuffer & buffer, uint32_t frameIndex) {

  vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[frameIndex], 0, nullptr);

  lout << "Updating push constants " << this << " " << model->getBindingDescription().size() << std::endl;

  glm::mat4 data = toGLMMatrx(getTransformationMatrix(transform));

  vkCmdPushConstants(buffer, shader->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), &data);

  model->bindForRender(buffer);
  VkDeviceSize offsets[] = {0};
  vkCmdDrawIndexed(buffer, model->getIndexCount(), 1, 0, 0, 0);


}

std::vector<VkDescriptorSet> & RenderElement::getDescriptorSets() {
  return this->descriptorSets;
}

std::vector<VmaAllocation> & RenderElement::getMemories() {
  return this->uniformBuffersMemory;
}

RenderElement * RenderElement::buildRenderElement(Viewport * view, std::shared_ptr<Model> model, std::shared_ptr<Material> material, Transform<float> & initTransform) {

  RenderElement * rElem = new RenderElement(view, model, material, view->getSwapchainSize(), initTransform);
  rElem->constructBuffers(view->getSwapchainSize());
  return rElem;

}
