#include "instancedrenderelement.h"

Transform<float> nullTransform;

InstancedRenderElement::InstancedRenderElement(Viewport * view, std::shared_ptr<Model> model, std::shared_ptr<Material> material, int scSize) : RenderElement(view, model, material, scSize, nullTransform, true) {

  instanceTransforms = std::vector<glm::mat4>(1);
  instances = std::unordered_map<uint32_t, InstanceInfo>(1);
  transforms = std::vector<Transform<float>>(1);

  transforms[0] = Transform<float>();

  instanceTransforms[0] = toGLMMatrx(getTransformationMatrix(transforms[0]));

  instanceCount = 1;

  std::cout << "Creating DynamicBuffer" << std::endl;

  this->instanceBuffer = new DynamicBuffer<glm::mat4>(state, instanceTransforms, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  std::cout << "Creating uniform buffers" << std::endl;
  this->createUniformBuffers(scSize, this->binds);

  std::cout << "Creating descriptor pool" << std::endl;

  std::cout << "Shader: " << this->shader << std::endl;

  //this->descPool = this->shader->setupDescriptorPool(scSize, binds);
  //this->descriptorSets = shader->createDescriptorSets(descPool, descSetLayout, this->binds, texture, scSize);

}

void InstancedRenderElement::constructBuffers(int scSize) {

  RenderElement::constructBuffers(scSize);
  this->instanceBuffer = new DynamicBuffer<glm::mat4>(state, instanceTransforms, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

}

void InstancedRenderElement::markBufferDirty() {
  this->instanceBufferDirty = true;
  this->handler.signalTransfer(this);
}

RenderElement::Instance InstancedRenderElement::addInstance(Transform<float> & trans) {
  uint32_t index = this->transforms.size();

  transforms.push_back(trans);
  instanceTransforms.push_back(toGLMMatrx(getTransformationMatrix(trans)));

  instances[index] = (InstanceInfo){index, index};
  this->markBufferDirty();

  this->instanceBuffer->recreate(instanceTransforms);

  instanceCount++;
  this->instanceCountUpdated = true;

  return (Instance) {index};
}

void InstancedRenderElement::updateInstance(Instance &instance, Transform<float> &trans) {

  //Check for existance of instance.
  if (this->instances.find(instance.id) == instances.end())
    return;

  InstanceInfo info = this->instances[instance.id];

  transformBufferMutex.lock();

  this->transforms[info.pos] = trans;
  this->instanceTransforms[info.pos] = toGLMMatrx(getTransformationMatrix(trans));

  transformBufferMutex.unlock();

  this->markBufferDirty();

}

void InstancedRenderElement::deleteInstance(Instance &instance) {

  transformBufferMutex.lock();

  ///No instances left
  if (!transforms.size()) {
    return;
  }

  //Info of instance to remove
  InstanceInfo info = this->instances[instance.id];
  uint32_t lastPos = instanceCount - 1;

  //remove instance from map
  this->instances.erase(this->instances.find(instance.id));

  //getting info for last element
  InstanceInfo * lastInfo;
  for (auto it : instances) {

    if (it.second.pos == lastPos) {
      lastInfo = &it.second;
      break;
    }

  }

  //Moving last element to override deleted one.
  transforms[info.pos] = transforms[lastPos];
  instanceTransforms[info.pos] = instanceTransforms[lastPos];
  lastInfo->pos = info.pos;

  //this->instanceBuffer->recreate(this->instanceTransforms);
  this->markBufferDirty();

  instanceCount--;
  //this->instanceCountUpdated = true;
  transformBufferMutex.unlock();

}

void InstancedRenderElement::recordTransfer(VkCommandBuffer &cmdBuffer) {

  std::cout << "Recording instance-transfer " << instanceTransforms.size() << std::endl;
  transformBufferMutex.lock();
  this->instanceBuffer->fill(instanceTransforms, cmdBuffer);
  transformBufferMutex.unlock();

}

void InstancedRenderElement::updateUniformBuffer(UniformBufferObject & obj,  uint32_t imageIndex) {

  void * data;
  vmaMapMemory(state.vmaAllocator, uniformBuffersMemory[imageIndex], &data);
  memcpy(data, &obj, sizeof(UniformBufferObject));
  vmaUnmapMemory(state.vmaAllocator, uniformBuffersMemory[imageIndex]);

  if (this->instanceCountUpdated) {

    this->instanceCountUpdated = false;

  }
  if (this->instanceBufferDirty) {

    this->instanceBufferDirty = false;

  }

}

bool InstancedRenderElement::needsDrawCmdUpdate() {
  return instanceCountUpdated;
}

void InstancedRenderElement::renderShaderless(VkCommandBuffer & buffer, uint32_t frameIndex) {

  vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[frameIndex], 0, nullptr);

  //std::cout << "InstancedRenderElement::renderShaderless" << std::endl;

  //std::cout << "Updating push constants" << std::endl;

  //glm::mat4 data = toGLMMatrx(getTransformationMatrix(transform));

  //vkCmdPushConstants(buffer, shader->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), &data);

  model->bindForRender(buffer);
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(buffer, 1, 1, &instanceBuffer->getBuffer(), offsets);
  vkCmdDrawIndexed(buffer, model->getIndexCount(), instanceCount, 0, 0, 0);
}

void InstancedRenderElement::render(VkCommandBuffer & buffer, uint32_t frameIndex) {

  //std::cout << "Pipeline (Instanced) " << pipeline << std::endl;
  vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
  vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[frameIndex], 0, nullptr);

  //std::cout << "InstancedRenderElement::render" << std::endl;

  //std::cout << "Updating push constants" << std::endl;

  //glm::mat4 data = toGLMMatrx(getTransformationMatrix(transform));

  //vkCmdPushConstants(buffer, shader->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), &data);

  model->bindForRender(buffer);
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(buffer, 1, 1, &instanceBuffer->getBuffer(), offsets);
  vkCmdDrawIndexed(buffer, model->getIndexCount(), instanceCount, 0, 0, 0);
}

