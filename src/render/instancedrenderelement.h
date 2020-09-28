#ifndef INSTANCED_RENDER_ELEMENT_H
#define INSTANCED_RENDER_ELEMENT_H

#include <mutex>
#include <unordered_map>
#include <memory>
#include <vector>
#include "renderelement.h"

class InstancedRenderElement : public RenderElement {

 public:

  InstancedRenderElement(Viewport * view, std::shared_ptr<Model> model, std::shared_ptr<Material> mat, int scSize);

  Instance addInstance(Transform<float> & pos) override;
  void updateInstance(Instance & instance, Transform<float> & newPos) override;
  void deleteInstance(Instance & instance) override;

  void recordTransfer(VkCommandBuffer & buffer) override;
  bool needsDrawCmdUpdate() override;
  void renderShaderless(VkCommandBuffer & buffer, uint32_t frameIndex) override;

  void updateUniformBuffer(UniformBufferObject & obj, uint32_t frameIndex) override;

 protected:

  bool instanceBufferDirty;
  bool instanceCountUpdated;

  void constructBuffers(int scSize) override;
  
 private:

  struct InstanceInfo {

    uint32_t id;
    uint32_t pos;

  };

  DynamicBuffer<glm::mat4> * instanceBuffer;
  std::vector<glm::mat4> instanceTransforms;
  std::unordered_map<uint32_t, InstanceInfo> instances;
  std::vector<Transform<float>> transforms;
  uint32_t instanceCount;

  std::mutex transformBufferMutex;
  

};

#endif
