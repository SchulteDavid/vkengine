#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <memory>

#include <glm/glm.hpp>

#include "render/util/vkutil.h"
#include "window.h"
#include "model.h"
#include "renderelement.h"
#include "camera.h"
#include "render/postprocessing.h"

#define VIEWPORT_MAX_LIGHT_COUNT 128

class ThreadedBufferManager {

public:

  struct BufferElement {

    uint32_t usageCount;
    VkCommandBuffer buffer;

  };

  ThreadedBufferManager();
  ThreadedBufferManager(unsigned int bufferCount, unsigned int frameCount, vkutil::VulkanState & state);

  /// returns the active buffer
  VkCommandBuffer getBufferForRender(uint32_t frameIndex);
  /// releases the active buffer back to the queue
  void releaseRenderBuffer(uint32_t frameIndex, bool internal=false);

  /// returns a buffer that can be recorded to
  BufferElement * getBufferForRecording();
  /// sets the next buffer to use.
  void setActiveBuffer(BufferElement * buffer);

  void printAttachedBuffers();

private:

  void pushBufferToUseable(BufferElement * buffer);

  std::mutex lock;
  std::condition_variable var;

  VkCommandPool bufferPool;
  std::vector<BufferElement> buffers;
  std::vector<BufferElement *> attachedBuffers;

  std::queue<BufferElement *> useableBuffers;
  BufferElement * activeBuffer;
  BufferElement * nextBuffer;

};

class Viewport : public MemoryTransferHandler {
public:
  Viewport(std::shared_ptr<Window> window, std::shared_ptr<Camera> camera, std::shared_ptr<Shader> defferedShader, std::vector<std::shared_ptr<PPEffect>> effects, std::shared_ptr<Texture> skyBox);
  virtual ~Viewport();

  struct LightData {

    int32_t activeCount;
    alignas(16) glm::vec4 position[VIEWPORT_MAX_LIGHT_COUNT];
    alignas(16) glm::vec4 color[VIEWPORT_MAX_LIGHT_COUNT];

  };

  struct CameraData;

  void drawFrame(bool updateElements = true);

  vkutil::VulkanState & getState();
  const VkRenderPass & getRenderpass();
  const VkExtent2D & getSwapchainExtent();
  unsigned int getSwapchainSize();

  void addRenderElement(std::shared_ptr<RenderElement> rElem);

  uint32_t addLight(glm::vec4 pos, glm::vec4 color);
  void updateLight(uint32_t index, glm::vec4 pos, glm::vec4 color);

  void manageMemoryTransfer();

  void createSecondaryBuffers();
  void renderIntoSecondary();

  std::shared_ptr<Camera> getCamera();

protected:

  void setupRenderPass();
  void createDefferedObjects();
  void destroyDefferedObjects();

  void createDefferedDescriptorSetLayout();
  void createDefferedDescriptorPool();
  void createDefferedDescriptorSets();

  void setupPostProcessingPipeline();
  void setupFramebuffers();

  void destroySwapChain();
  void recreateSwapChain();

  void recordCommandBuffers();
  void setupCommandBuffers();
  void createSyncObjects();

  void createTransferCommandBuffer();

  void prepareRenderElements();

  void updateUniformBuffer(uint32_t frameIndex);

  void recordSingleBuffer(VkCommandBuffer & buffer, unsigned int frameIndex);

private:

  struct SwapchainInfo : vkutil::SwapChain {

    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkImageView> imageViews;


  } swapchain;

  vkutil::VulkanState & state;

  std::shared_ptr<Camera> camera;
  bool framebufferResized;

  Window * window;
  VkRenderPass renderPass;

  VkImage depthImage;
  VkImageView depthImageView;
  VmaAllocation depthImageMemory;

  VkImage gBufferImage;
  VkImageView gBufferImageView;
  VmaAllocation gBufferImageMemory;

  VkImage nBufferImage;
  VkImageView nBufferImageView;
  VmaAllocation nBufferImageMemory;

  VkImage aBufferImage;
  VkImageView aBufferImageView;
  VmaAllocation aBufferImageMemory;

  std::vector<VkBuffer> defferedLightBuffers;
  std::vector<VmaAllocation> defferedLightBuffersMemory;
  std::vector<VkBuffer> defferedCameraBuffers;
  std::vector<VmaAllocation> defferedCameraBuffersMemory;

  VkDescriptorSetLayout defferedDescLayout;
  VkDescriptorPool defferedDescPool;
  std::vector<VkDescriptorSet> defferedDescSets;

  std::shared_ptr<Shader> defferedShader;

  VkPipelineLayout defferedPipelineLayout;
  VkPipeline defferedPipeline;

  std::vector<std::shared_ptr<PPEffect>> ppEffects;
  std::vector<VkImage> ppImages;
  std::vector<VmaAllocation> ppImageMemories;
  std::vector<VkImageView> ppImageViews;

  std::vector<VkCommandBuffer> commandBuffers;
  VkCommandBuffer transferCmdBuffer;

  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  int frameIndex;
  std::vector<VkFence> inFlightFences;
  VkFence transferFence;

  std::shared_ptr<Model> ppBufferModel;

  uint32_t lightDataModified;
  LightData lights;
  unsigned int lightIndex;

  UniformBufferObject ubo;

  std::vector<std::shared_ptr<RenderElement>> renderElements;
  std::unordered_map<Shader *, std::vector<std::shared_ptr<RenderElement>>> renderElementsByShader;

  std::queue<SwapchainInfo> destroyableSwapchains;

  bool isLightDataModified(uint32_t imageIndex);
  void markLightDataCorrect(uint32_t imageIndex);

  ThreadedBufferManager * bufferManager;

  std::shared_ptr<Texture> skyBox;

};

#endif // VIEWPORT_H
