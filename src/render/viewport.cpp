#include "viewport.h"

#include <iostream>
#include <chrono>

#define MAX_FRAMES_IN_FLIGHT 3

#include "util/debug/trace_exception.h"
#include "util/debug/logger.h"
#include "util/vk_trace_exception.h"

struct Viewport::CameraData {

  alignas(16) glm::mat4 view;
  glm::mat4 projection;

};

std::vector<Model::Vertex> viewModelData = {

					    {glm::vec3(-1, -1, 0), glm::vec3(0, 0, 1), glm::vec3(1, 0, 0), glm::vec2(0, 0), 0},
					    {glm::vec3( 1, -1, 0),  glm::vec3(0, 0, 1), glm::vec3(1, 0, 0), glm::vec2(1, 0), 0},
					    {glm::vec3( 1,  1, 0),   glm::vec3(0, 0, 1), glm::vec3(1, 0, 0), glm::vec2(1, 1), 0},
					    {glm::vec3(-1,  1, 0),  glm::vec3(0, 0, 1), glm::vec3(1, 0, 0), glm::vec2(0, 1), 0},

};

std::vector<uint16_t> viewModelIndices = {
					  2, 3, 0,
					  0, 1, 2,
					  2, 3, 0,
					  0, 1, 2,
};

Viewport::Viewport(std::shared_ptr<Window> window, std::shared_ptr<Camera> camera, std::shared_ptr<Shader> defferedShader, std::vector<std::shared_ptr<PPEffect>> effects, std::shared_ptr<Texture> sb) : state(window->getState()) {

  bufferManager = nullptr;
  this->skyBox = sb;
  this->skyBox->transitionLayout(state, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  this->camera = camera;
  this->lightIndex = 0;
  this->defferedShader = defferedShader;
  this->ppEffects = effects;
  this->framebufferResized = false;

  this->frameIndex = 0;
  this->framebufferResized = false;

  lout << "Creating swapchain" << std::endl;

  vkutil::SwapChain tmpChain = vkutil::createSwapchain(window->getPhysicalDevice(), state.device, window->getSurface(), window->getGlfwWindow());

  swapchain.chain = tmpChain.chain;
  swapchain.extent = tmpChain.extent;
  swapchain.format = tmpChain.format;
  swapchain.images = tmpChain.images;

  lout << "setting up render pass" << std::endl;

  setupRenderPass();

  /** Creating depth resources **/
  VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
  vkutil::createImage(state.vmaAllocator, state.device, swapchain.extent.width, swapchain.extent.height, 1, 1, depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
  depthImageView = vkutil::createImageView(state.device, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1, VK_IMAGE_VIEW_TYPE_2D, 1);
  vkutil::transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, state.graphicsCommandPool, state.device, state.graphicsQueue);

  swapchain.imageViews = vkutil::createSwapchainImageViews(swapchain.images, swapchain.format, state.device);

  createDefferedDescriptorSetLayout();
  createDefferedObjects();
  setupPostProcessingPipeline();

  for (uint32_t i = 0; i < ppEffects.size(); ++i) {

    ppEffects[i]->createDescriptorSetLayout(state);
    // subpass has to be offset by 2 for object-pass and deffered-lighting
    ppEffects[i]->setupPipeline(state, swapchain, renderPass, i+2);
  }

  setupFramebuffers();
  createDefferedDescriptorPool();
  createDefferedDescriptorSets();

  for (uint32_t i = 0; i < ppEffects.size(); ++i) {
    ppEffects[i]->createDescriptorPool(state, swapchain);
    ppEffects[i]->createDescriptorSet(state, swapchain, ppImageViews[i], depthImageView);
  }
  
  
  setupCommandBuffers();
  createTransferCommandBuffer();
  createSyncObjects();

  //ppBufferModel = std::shared_ptr<Model>(Model::loadFromFile(state, "resources/models/quad.ply"));
  ppBufferModel = std::shared_ptr<Model>(new Model(state, viewModelData, viewModelIndices));

  ppBufferModel->uploadToGPU(state.device, state.graphicsCommandPool, state.graphicsQueue);


  //createSecondaryBuffers();

  //recordCommandBuffers();

  this->lightDataModified = false;

}

Viewport::~Viewport() {
  for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    vkWaitForFences(state.device, 1, &inFlightFences[i], VK_TRUE, std::numeric_limits<uint64_t>::max());

  if (bufferManager) delete bufferManager;

}

void Viewport::createSyncObjects() {

  imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {

    if (vkCreateSemaphore(state.device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS || vkCreateSemaphore(state.device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {
      throw dbg::trace_exception("Unable to create semaphores");
    }

    lout << "Creating fence for frame " << i << std::endl;
    if (vkCreateFence(state.device, &fenceInfo, nullptr, &this->inFlightFences[i]) != VK_SUCCESS)
      throw dbg::trace_exception("Unable to create fence");
    //vkResetFences(state.device, 1, &inFlightFences[i]);

  }

}

void Viewport::createTransferCommandBuffer() {

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = state.transferCommandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  if (vkAllocateCommandBuffers(state.device, &allocInfo, &transferCmdBuffer) != VK_SUCCESS)
    throw dbg::trace_exception("Unable to allocate command buffer");

  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  vkCreateFence(state.device, &fenceInfo, nullptr, &transferFence);

}

void Viewport::prepareRenderElements() {

  bool needsUpdate = false;

  for (unsigned int i = 0; i < renderElements.size(); ++i) {
    needsUpdate |= renderElements[i]->needsDrawCmdUpdate();
    if (needsUpdate) break;
  }

  if (needsUpdate) {

    //state.graphicsQueueMutex.lock();
    //vkDeviceWaitIdle(state.device);
    for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
      vkWaitForFences(state.device, 1, &inFlightFences[i], VK_TRUE, std::numeric_limits<uint64_t>::max());
    vkFreeCommandBuffers(state.device, state.graphicsCommandPool, commandBuffers.size(), commandBuffers.data());
    setupCommandBuffers();
    recordCommandBuffers();
    //state.graphicsQueueMutex.unlock();


  }

}

void Viewport::manageMemoryTransfer() {

  if (this->framebufferResized) {
    return;
  }

  if (this->hasPendingTransfer()) {

    //lout << "Waiting for transfer fences" << std::endl;
    vkWaitForFences(state.device, 1, &transferFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
    vkResetFences(state.device, 1, &transferFence);

    lout << "Recording transfer" << std::endl;
    this->recordTransfer(transferCmdBuffer);

    VkSubmitInfo transferSubmit = {};
    transferSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    transferSubmit.commandBufferCount = 1;
    transferSubmit.pCommandBuffers = &transferCmdBuffer;
    transferSubmit.signalSemaphoreCount = 0;
    transferSubmit.waitSemaphoreCount = 0;

    state.transferQueue.lock();
    vkQueueSubmit(state.transferQueue.q, 1, &transferSubmit, transferFence);
    state.transferQueue.unlock();

  }

}

void Viewport::drawFrame(bool updateElements) {

  static auto startRenderTime = std::chrono::high_resolution_clock::now();

  if (updateElements)
    prepareRenderElements();

  /// Keep this like this, this computes the mathematical modulo, no negative results.
  /// This will always keep the frame order correct.
  int32_t releaseFrameIndex = ((frameIndex - 1) + MAX_FRAMES_IN_FLIGHT) % MAX_FRAMES_IN_FLIGHT;

  //std::cout << "Waiting for frame " << releaseFrameIndex << " to be finished" << std::endl;
  vkWaitForFences(state.device, 1, &inFlightFences[releaseFrameIndex], VK_TRUE, std::numeric_limits<uint64_t>::max());

  if (bufferManager) {
    //std::cout << "Releasing buffer for frameIndex " << releaseFrameIndex << std::endl;
    bufferManager->releaseRenderBuffer(releaseFrameIndex);
    //if (bufferManager) {
    //lout << "Buffer layout: " << std::endl;
    //bufferManager->printAttachedBuffers();
    //lout << std::endl;
    //}
  }
  
  //vkDeviceWaitIdle(state.device);

  uint32_t imageIndex = 0;
  VkResult result = vkAcquireNextImageKHR(state.device, swapchain.chain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex);

  //std::cout << "imageIndex " << imageIndex << "  " << commandBuffers.size() << std::endl;
  if (imageIndex >= commandBuffers.size()) {
    std::cerr << "WTF? " << imageIndex << " " << commandBuffers.size() << std::endl;
  }

  if (!this->destroyableSwapchains.empty()) {

    SwapchainInfo i = destroyableSwapchains.front();

  }

  //std::cout << "Recording  single buffer" << std::endl;
  
  recordSingleBuffer(commandBuffers[imageIndex], imageIndex);

  //std::cout << "Recording done " << std::endl;

  switch(result) {

  case VK_SUBOPTIMAL_KHR:
  case VK_ERROR_OUT_OF_DATE_KHR:
    destroySwapChain();
    recreateSwapChain();
    framebufferResized = false;
    return;

  case VK_SUCCESS:
    break;

  default:
    throw dbg::trace_exception("Unable to fetch image from swap chain");

  }

  updateUniformBuffer(imageIndex);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[frameIndex]};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

  VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[frameIndex]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  vkResetFences(state.device, 1, &inFlightFences[frameIndex]);

  state.graphicsQueue.lock();
  if (VkResult res = vkQueueSubmit(state.graphicsQueue.q, 1, &submitInfo, inFlightFences[frameIndex]))
    throw vkutil::vk_trace_exception("Unable to submit command buffer", res);
  state.graphicsQueue.unlock();

  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;
  VkSwapchainKHR swapChains[] = {swapchain.chain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.pResults = nullptr;

  state.graphicsQueue.lock();
  vkQueuePresentKHR(state.presentQueue.q, &presentInfo);
  state.graphicsQueue.unlock();

  frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;

  if (!frameIndex) {
    double duration = std::chrono::duration<double, std::chrono::milliseconds::period>(std::chrono::high_resolution_clock::now() - startRenderTime).count();
    lout << "Frame time: " << duration << "ms => fps: " << (1000.0 / duration) << std::endl;
  }

  startRenderTime = std::chrono::high_resolution_clock::now();

}

std::shared_ptr<Camera> Viewport::getCamera() {
  return camera;
}

bool Viewport::isLightDataModified(uint32_t imageIndex) {
  return true;
}

void Viewport::markLightDataCorrect(uint32_t imageIndex) {
  this->lightDataModified &= 0xffffffff ^ (0x1 << imageIndex);
}

void Viewport::updateUniformBuffer(uint32_t imageIndex) {

  //auto startTime = std::chrono::high_resolution_clock::now();

  ubo.view = this->camera->getView();
  ubo.proj = this->camera->getProjection();

  for (unsigned int i = 0; i < renderElements.size(); ++i) {

    renderElements[i]->updateUniformBuffer(ubo, imageIndex);

  }

  void * data;
  if (isLightDataModified(imageIndex)) {
    LightData * lData;
    vmaMapMemory(state.vmaAllocator, defferedLightBuffersMemory[imageIndex], (void **)&lData);
    //memcpy(data, &lights, sizeof(LightData));
    *lData = lights;
    vmaUnmapMemory(state.vmaAllocator, defferedLightBuffersMemory[imageIndex]);
    //markLightDataCorrect(imageIndex);
  }

  vmaMapMemory(state.vmaAllocator, defferedCameraBuffersMemory[imageIndex], &data);
  CameraData * camData = (CameraData *) data;

  Transform<float> camTrans = camera->getTransform();
  camData->view = toGLMMatrix(getTransformationMatrix<float>(camTrans));
  camData->projection = camera->getProjection();
  
  vmaUnmapMemory(state.vmaAllocator, defferedCameraBuffersMemory[imageIndex]);

  
}


void Viewport::setupRenderPass() {

  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format = swapchain.format;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentDescription gAttachment = {};
  gAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  gAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  gAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  gAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  gAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  gAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  gAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  gAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription nAttachment = {};
  nAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  nAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  nAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  nAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  nAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  nAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  nAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  nAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription aAttachment = {};
  aAttachment.format = swapchain.format;
  aAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  aAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  aAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  aAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  aAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  aAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  aAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription depthAttachment = {};
  depthAttachment.format = VK_FORMAT_D32_SFLOAT;
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = ppEffects.size() ? 5 : 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentRef = {};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference gAttachmentRef = {};
  gAttachmentRef.attachment = 2;
  gAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference nAttachmentRef = {};
  nAttachmentRef.attachment = 3;
  nAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference aAttachmentRef = {};
  aAttachmentRef.attachment = 4;
  aAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference gInputRef = {};
  gInputRef.attachment = 2;
  gInputRef.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkAttachmentReference nInputRef = {};
  nInputRef.attachment = 3;
  nInputRef.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkAttachmentReference aInputRef = {};
  aInputRef.attachment = 4;
  aInputRef.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  std::array<VkAttachmentReference, 3> outputRefs = {gAttachmentRef, nAttachmentRef, aAttachmentRef};
  std::array<VkAttachmentReference, 3> inputRefs  = {gInputRef, nInputRef, aInputRef};

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = outputRefs.size();
  subpass.pColorAttachments = outputRefs.data();
  subpass.pDepthStencilAttachment = &depthAttachmentRef;

  VkSubpassDescription subpass2 = {};
  subpass2.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass2.colorAttachmentCount = 1;
  subpass2.pColorAttachments = &colorAttachmentRef;
  subpass2.inputAttachmentCount = inputRefs.size();
  subpass2.pInputAttachments = inputRefs.data();

  std::vector<VkSubpassDescription> ppDescriptions(ppEffects.size());
  std::vector<VkSubpassDependency> ppDependencies(ppEffects.size());
  std::vector<VkAttachmentDescription> ppAttachmentDescs(ppEffects.size());
  std::vector<VkAttachmentReference> ppAttachmentRefs(3 * ppEffects.size());

  for (uint32_t i = 0; i < ppEffects.size(); ++i) {

    VkAttachmentDescription ppAttachment = {};
    ppAttachment.format = swapchain.format;
    ppAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    ppAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ppAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ppAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ppAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ppAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ppAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    ppAttachmentDescs[i] = ppAttachment;

    ppAttachmentRefs[i*3] = {};
    ppAttachmentRefs[i*3].attachment = i+5;
    ppAttachmentRefs[i*3].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    ppAttachmentRefs[i*3+1] = {};
    ppAttachmentRefs[i*3+1].attachment = 2;
    ppAttachmentRefs[i*3+1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    ppAttachmentRefs[i * 3 + 2] = {};
    ppAttachmentRefs[i*3+2].attachment = (i == ppEffects.size()-1 ? 0 : i+6);
    ppAttachmentRefs[i*3+2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    ppDescriptions[i].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    ppDescriptions[i].colorAttachmentCount = 1;
    ppDescriptions[i].inputAttachmentCount = 1;
    ppDescriptions[i].pColorAttachments = &ppAttachmentRefs[i*3+2];
    ppDescriptions[i].pInputAttachments = &ppAttachmentRefs[i*3];

    ppDependencies[i].srcSubpass = i+1;
    ppDependencies[i].dstSubpass = i+2;
    ppDependencies[i].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    ppDependencies[i].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    ppDependencies[i].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    ppDependencies[i].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
  }

  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkSubpassDependency dependency2 = {};
  dependency2.srcSubpass = 0;
  dependency2.dstSubpass = 1;
  dependency2.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency2.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependency2.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency2.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  std::vector<VkAttachmentDescription> attachments = {colorAttachment, depthAttachment, gAttachment, nAttachment, aAttachment};
  attachments.insert(attachments.end(), ppAttachmentDescs.begin(), ppAttachmentDescs.end());

  std::vector<VkSubpassDescription> subpasses = {subpass, subpass2};
  subpasses.insert(subpasses.end(), ppDescriptions.begin(), ppDescriptions.end());
  std::vector<VkSubpassDependency> dependencies = {dependency, dependency2};
  dependencies.insert(dependencies.end(), ppDependencies.begin(), ppDependencies.end());

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = attachments.size();
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = subpasses.size();
  renderPassInfo.pSubpasses = subpasses.data();
  renderPassInfo.dependencyCount = dependencies.size();
  renderPassInfo.pDependencies = dependencies.data();

  if (vkCreateRenderPass(state.device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
    throw dbg::trace_exception("Unable to create renderpass");

}

void Viewport::createDefferedObjects() {

  vkutil::createImage(state.vmaAllocator, state.device, swapchain.extent.width, swapchain.extent.height, 1, 1, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gBufferImage, gBufferImageMemory);
  gBufferImageView = vkutil::createImageView(state.device, gBufferImage, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_IMAGE_VIEW_TYPE_2D, 1);

  vkutil::createImage(state.vmaAllocator, state.device, swapchain.extent.width, swapchain.extent.height, 1, 1, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, nBufferImage, nBufferImageMemory);
  nBufferImageView = vkutil::createImageView(state.device, nBufferImage, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_IMAGE_VIEW_TYPE_2D, 1);

  vkutil::createImage(state.vmaAllocator, state.device, swapchain.extent.width, swapchain.extent.height, 1, 1, swapchain.format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, aBufferImage, aBufferImageMemory);
  aBufferImageView = vkutil::createImageView(state.device, aBufferImage, swapchain.format, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_IMAGE_VIEW_TYPE_2D, 1);

  this->ppImages.resize(ppEffects.size());
  this->ppImageViews.resize(ppEffects.size());
  this->ppImageMemories.resize(ppEffects.size());
  
  for (unsigned int i = 0; i < ppEffects.size(); ++i) {

    vkutil::createImage(state.vmaAllocator, state.device, swapchain.extent.width, swapchain.extent.height, 1, 1, swapchain.format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ppImages[i], ppImageMemories[i]);
    ppImageViews[i] = vkutil::createImageView(state.device, ppImages[i], swapchain.format, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_IMAGE_VIEW_TYPE_2D, 1);
    
  }

  VkDeviceSize lightSize = sizeof(LightData);
  VkDeviceSize cameraSize = sizeof(CameraData);

  defferedLightBuffers.resize(swapchain.images.size());
  defferedLightBuffersMemory.resize(swapchain.images.size());

  defferedCameraBuffers.resize(swapchain.images.size());
  defferedCameraBuffersMemory.resize(swapchain.images.size());

  lout << "Images created" << std::endl;

  for (unsigned int i = 0; i < swapchain.images.size(); ++i) {

    {
      VkBufferCreateInfo stBufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
      stBufferCreateInfo.size = lightSize;
      stBufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
      stBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      VmaAllocationCreateInfo stAllocCreateInfo = {};
      stAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
      stAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

      VmaAllocationInfo stagingBufferAllocInfo = {};

      lout << "Creating light Buffer" << std::endl;

      vmaCreateBuffer(state.vmaAllocator, &stBufferCreateInfo, &stAllocCreateInfo, &defferedLightBuffers[i], &defferedLightBuffersMemory[i], &stagingBufferAllocInfo);
    }
    {
      VkBufferCreateInfo stBufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
      stBufferCreateInfo.size = cameraSize;
      stBufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
      stBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      VmaAllocationCreateInfo stAllocCreateInfo = {};
      stAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
      stAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

      VmaAllocationInfo stagingBufferAllocInfo = {};

      lout << "Creating camera Buffer" << std::endl;

      vmaCreateBuffer(state.vmaAllocator, &stBufferCreateInfo, &stAllocCreateInfo, &defferedCameraBuffers[i], &defferedCameraBuffersMemory[i], &stagingBufferAllocInfo);
    }
    //StorageBuffer::createBuffer(vmaAllocator, cameraSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, ppCameraBuffers[i], ppCameraBuffersMemory[i]);

  }

}

void Viewport::destroyDefferedObjects() {

  vmaDestroyImage(state.vmaAllocator, gBufferImage, gBufferImageMemory);
  vkDestroyImageView(state.device, gBufferImageView, nullptr);

  vmaDestroyImage(state.vmaAllocator, nBufferImage, nBufferImageMemory);
  vkDestroyImageView(state.device, nBufferImageView, nullptr);

  vmaDestroyImage(state.vmaAllocator, aBufferImage, aBufferImageMemory);
  vkDestroyImageView(state.device, aBufferImageView, nullptr);

  for (unsigned int i = 0; i < swapchain.images.size(); ++i) {

    vmaDestroyBuffer(state.vmaAllocator, defferedLightBuffers[i], defferedLightBuffersMemory[i]);
    vmaDestroyBuffer(state.vmaAllocator, defferedCameraBuffers[i], defferedCameraBuffersMemory[i]);

  }

}

void Viewport::addRenderElement(std::shared_ptr<RenderElement> rElem) {
  this->renderElements.push_back(rElem);

  if (this->renderElementsByShader.find(rElem->getShader()) == renderElementsByShader.end()) {
    renderElementsByShader[rElem->getShader()] = std::vector<std::shared_ptr<RenderElement>>(1);
    renderElementsByShader[rElem->getShader()][0] = rElem;
  } else {
    renderElementsByShader[rElem->getShader()].push_back(rElem);
  }

}

void Viewport::setupPostProcessingPipeline() {

  /** shaders **/

  lout << "Creating ppPipeline" << std::endl;

  std::vector<vkutil::ShaderInputDescription> shaders = defferedShader->getShaderInputDescriptions();

  std::vector<VkPipelineShaderStageCreateInfo> shaderStages(shaders.size());
  for (unsigned int i = 0; i < shaders.size(); ++i) {
    
    shaderStages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[i].module = shaders[i].module;
    shaderStages[i].stage = shaders[i].usage;
    shaderStages[i].pName = shaders[i].entryName;
    
  }

  /** fixed function **/

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  VkVertexInputBindingDescription description = {};
  description.binding = 0;
  description.stride = sizeof(Model::Vertex);
  description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  std::vector<VkVertexInputAttributeDescription> descriptions(3);

  descriptions[0].binding = 0;
  descriptions[0].location = 0;
  descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  descriptions[0].offset = offsetof(Model::Vertex, pos);

  descriptions[1].binding = 0;
  descriptions[1].location = 1;
  descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  descriptions[1].offset = offsetof(Model::Vertex, normal);

  descriptions[2].binding = 0;
  descriptions[2].location = 2;
  descriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
  descriptions[2].offset = offsetof(Model::Vertex, tangent);

  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &description;
  vertexInputInfo.vertexAttributeDescriptionCount = descriptions.size();
  vertexInputInfo.pVertexAttributeDescriptions = descriptions.data();

  vkutil::VertexInputDescriptions inputDescriptions;
  inputDescriptions.binding = {description};
  inputDescriptions.attributes = descriptions;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  /* Viewport */

  VkViewport viewport = {};
  viewport.x = 0;
  viewport.y = 0;
  viewport.width = swapchain.extent.width;
  viewport.height = swapchain.extent.height;
  viewport.minDepth = 0.0;
  viewport.maxDepth = 1.0;

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = swapchain.extent;

  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  /* Rasterizer */

  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0;
  rasterizer.cullMode = VK_CULL_MODE_NONE;
  //rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0;
  rasterizer.depthBiasClamp = 0.0;
  rasterizer.depthBiasSlopeFactor = 0.0;

  /* Multisampling */

  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f;
  multisampling.pSampleMask = nullptr;
  multisampling.alphaToCoverageEnable = VK_FALSE;
  multisampling.alphaToOneEnable = VK_FALSE;

  /* color blending */

  // For one framebuffer
  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  //std::array<VkPipelineColorBlendAttachmentState, 4> cbAttachments = {colorBlendAttachment, colorBlendAttachment, colorBlendAttachment, colorBlendAttachment};

  // global configuration
  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  VkPipelineDepthStencilStateCreateInfo depthStencil = {};
  depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.minDepthBounds = 0.0f;
  depthStencil.maxDepthBounds = 1.0f;
  depthStencil.stencilTestEnable = VK_FALSE;
  depthStencil.front = {};
  depthStencil.back = {};

  /* Uniform layout */


  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &defferedDescLayout; ///Get from Shader object
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = nullptr;

  if (vkCreatePipelineLayout(state.device, &pipelineLayoutInfo, nullptr, &defferedPipelineLayout) != VK_SUCCESS)
    throw dbg::trace_exception("Unable to create pipeline layout");


  /** Creating the pipeline **/

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = shaderStages.size();
  pipelineInfo.pStages = shaderStages.data();
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = nullptr; // change for dynamic state
  pipelineInfo.layout = defferedPipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 1;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex = -1;

  if (vkCreateGraphicsPipelines(state.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &defferedPipeline) != VK_SUCCESS)
    throw dbg::trace_exception("Unable to create pipeline");

}

void Viewport::createDefferedDescriptorSetLayout() {

  std::array<VkDescriptorSetLayoutBinding, 6> bindings;
  bindings[0].binding = 0;
  bindings[0].descriptorCount = 1;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
  bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  bindings[1].binding = 1;
  bindings[1].descriptorCount = 1;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
  bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  bindings[2].binding = 2;
  bindings[2].descriptorCount = 1;
  bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
  bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  bindings[3].binding = 3;
  bindings[3].descriptorCount = 1;
  bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  bindings[4].binding = 4;
  bindings[4].descriptorCount = 1;
  bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  bindings[5].binding = 5;
  bindings[5].descriptorCount = 1;
  bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  bindings[5].pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.pBindings = bindings.data();
  layoutInfo.bindingCount = bindings.size();

  if (vkCreateDescriptorSetLayout(state.device, &layoutInfo, nullptr, &defferedDescLayout) != VK_SUCCESS)
    throw dbg::trace_exception("could not create descriptor set layout.");

}

void Viewport::createDefferedDescriptorPool() {

  VkDescriptorPoolSize samplerSize = {};
  samplerSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
  samplerSize.descriptorCount = swapchain.images.size();

  VkDescriptorPoolSize lightSize = {};
  lightSize.descriptorCount = swapchain.images.size();
  lightSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

  VkDescriptorPoolSize cameraSize = {};
  cameraSize.descriptorCount = swapchain.images.size();
  cameraSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

  VkDescriptorPoolSize cubemapSize = {};
  cubemapSize.descriptorCount = swapchain.images.size();
  cubemapSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

  VkDescriptorPoolSize sizes[] = {
      samplerSize, samplerSize, samplerSize, lightSize, cameraSize, cubemapSize
  };

  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 6;
  poolInfo.pPoolSizes = sizes;
  poolInfo.maxSets = swapchain.images.size();

  if (vkCreateDescriptorPool(state.device, &poolInfo, nullptr, &defferedDescPool) != VK_SUCCESS)
    throw dbg::trace_exception("Unable to create descriptor pool");

}

void Viewport::createDefferedDescriptorSets() {

  std::vector<VkDescriptorSetLayout> layouts(swapchain.images.size(), defferedDescLayout);
  this->defferedDescSets.resize(swapchain.images.size());

  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = defferedDescPool;
  allocInfo.descriptorSetCount = swapchain.images.size();
  allocInfo.pSetLayouts = layouts.data();

  if (vkAllocateDescriptorSets(state.device, &allocInfo, defferedDescSets.data()) != VK_SUCCESS)
    throw dbg::trace_exception("Unable to allocate descriptor sets");

  for (unsigned int i = 0; i < swapchain.images.size(); ++i) {

    std::array<VkWriteDescriptorSet, 6> descriptorWrites = {};

    VkDescriptorImageInfo gInfo;
    gInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    gInfo.imageView = gBufferImageView;
    gInfo.sampler = VK_NULL_HANDLE;

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = defferedDescSets[i];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = nullptr;
    descriptorWrites[0].pImageInfo = &gInfo;
    descriptorWrites[0].pTexelBufferView = nullptr;
    descriptorWrites[0].pNext = nullptr;

    VkDescriptorImageInfo nInfo;
    nInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    nInfo.imageView = nBufferImageView;
    nInfo.sampler = VK_NULL_HANDLE;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = defferedDescSets[i];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = nullptr;
    descriptorWrites[1].pImageInfo = &nInfo;
    descriptorWrites[1].pTexelBufferView = nullptr;
    descriptorWrites[1].pNext = nullptr;

    VkDescriptorImageInfo aInfo;
    aInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    aInfo.imageView = aBufferImageView;
    aInfo.sampler = VK_NULL_HANDLE;

    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = defferedDescSets[i];
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pBufferInfo = nullptr;
    descriptorWrites[2].pImageInfo = &aInfo;
    descriptorWrites[2].pTexelBufferView = nullptr;
    descriptorWrites[2].pNext = nullptr;

    VkDescriptorBufferInfo lightInfo = {};
    lightInfo.buffer = this->defferedLightBuffers[i];
    lightInfo.offset = 0;
    lightInfo.range = sizeof(LightData);

    descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[3].dstSet = defferedDescSets[i];
    descriptorWrites[3].dstBinding = 3;
    descriptorWrites[3].dstArrayElement = 0;
    descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[3].descriptorCount = 1;
    descriptorWrites[3].pBufferInfo = &lightInfo;
    descriptorWrites[3].pImageInfo = nullptr;
    descriptorWrites[3].pTexelBufferView = nullptr;
    descriptorWrites[3].pNext = nullptr;

    VkDescriptorBufferInfo cameraInfo = {};
    cameraInfo.buffer = this->defferedCameraBuffers[i];
    cameraInfo.offset = 0;
    cameraInfo.range = sizeof(CameraData);

    descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[4].dstSet = defferedDescSets[i];
    descriptorWrites[4].dstBinding = 4;
    descriptorWrites[4].dstArrayElement = 0;
    descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[4].descriptorCount = 1;
    descriptorWrites[4].pBufferInfo = &cameraInfo;
    descriptorWrites[4].pImageInfo = nullptr;
    descriptorWrites[4].pTexelBufferView = nullptr;
    descriptorWrites[4].pNext = nullptr;

    VkDescriptorImageInfo cubeMapInfo = {};
    cubeMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    cubeMapInfo.imageView = skyBox->getView();
    cubeMapInfo.sampler = skyBox->getSampler();

    descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[5].dstSet = defferedDescSets[i];
    descriptorWrites[5].dstBinding = 5;
    descriptorWrites[5].dstArrayElement = 0;
    descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[5].descriptorCount = 1;
    descriptorWrites[5].pBufferInfo = nullptr;
    descriptorWrites[5].pImageInfo = &cubeMapInfo;
    descriptorWrites[5].pTexelBufferView = nullptr;
    descriptorWrites[5].pNext = nullptr;

    vkUpdateDescriptorSets(state.device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
  }

}

const VkRenderPass & Viewport::getRenderpass() {
  return renderPass;
}

const VkExtent2D & Viewport::getSwapchainExtent() {
  return swapchain.extent;
}

unsigned int Viewport::getSwapchainSize() {
  return swapchain.imageViews.size();
}

void Viewport::setupFramebuffers() {

  swapchain.framebuffers.resize(swapchain.imageViews.size());

  lout << "SwapChainSize " << swapchain.imageViews.size() << std::endl;

  for (unsigned int i = 0; i < swapchain.imageViews.size(); ++i) {

    std::vector<VkImageView> attachments = {swapchain.imageViews[i], depthImageView, gBufferImageView, nBufferImageView, aBufferImageView};
    attachments.insert(attachments.end(), ppImageViews.begin(), ppImageViews.end());

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = attachments.size();
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = swapchain.extent.width;
    framebufferInfo.height = swapchain.extent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(state.device, &framebufferInfo, nullptr, &swapchain.framebuffers[i]) != VK_SUCCESS)
      throw dbg::trace_exception("Unable to create framebuffer");

  }

  if (this->camera)
    this->camera->updateProjection(70.0, 0.01, 100.0, (float) swapchain.extent.width / (float) swapchain.extent.height);

}

void Viewport::destroySwapChain() {

  state.graphicsQueue.lock();
  vkQueueWaitIdle(state.graphicsQueue.q);
  state.graphicsQueue.unlock();

  destroyDefferedObjects();

  lout << "PP Objects destroyed" << std::endl;

  for (auto framebuffer : swapchain.framebuffers) {
    vkDestroyFramebuffer(state.device, framebuffer, nullptr);
  }

  lout << "Destroyed framebuffers" << std::endl;

  vkFreeCommandBuffers(state.device, state.graphicsCommandPool, commandBuffers.size(), commandBuffers.data());

  vkDestroyRenderPass(state.device, renderPass, nullptr);

  lout << "Destroyed renderpass" << std::endl;

  vkDestroyImageView(state.device, depthImageView, nullptr);
  vmaDestroyImage(state.vmaAllocator, depthImage, depthImageMemory);

  lout << "Destroyed depth image" << std::endl;

  for (const VkImageView & v : swapchain.imageViews) {
    vkDestroyImageView(state.device, v, nullptr);
  }

  for (const VkImage & i : swapchain.images) {
    vkDestroyImage(state.device, i, nullptr);
  }

  lout << "Destroyed images" << std::endl;

  destroyableSwapchains.push(swapchain);

  for (unsigned int i = 0; i < renderElements.size(); ++i) {
    this->renderElements[i]->destroyUniformBuffers(swapchain);
  }

  lout << "Destroyed uniform buffers" << std::endl;

}

void Viewport::recreateSwapChain() {

  lout << "Recreating swapchain" << std::endl;

  //state.graphicsQueueMutex.lock();

  vkutil::SwapChain tmpChain = vkutil::createSwapchain(state.physicalDevice, state.device, state.surface, state.glfwWindow);
  this->swapchain.chain = tmpChain.chain;
  this->swapchain.extent = tmpChain.extent;
  this->swapchain.format = tmpChain.format;
  this->swapchain.images = tmpChain.images;
  this->swapchain.imageViews = vkutil::createSwapchainImageViews(swapchain.images, swapchain.format, state.device);

  this->setupRenderPass();

  VkFormat depthFormat = VK_FORMAT_D32_SFLOAT; /// <- this can be chosen by a function later

  vkutil::createImage(state.vmaAllocator, state.device, swapchain.extent.width, swapchain.extent.height, 1, 1, depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
  lout << "Creating depth image view" << std::endl;
  depthImageView = vkutil::createImageView(state.device, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

  vkutil::transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, state.graphicsCommandPool, state.device, state.graphicsQueue);

  lout << "Creating PP objects" << std::endl;
  createDefferedObjects();

  this->setupFramebuffers();

  lout << "Done creating Framebuffers" << std::endl;

  for (unsigned int i = 0; i < renderElements.size(); ++i) {
    renderElements[i]->recreateResources(renderPass, swapchain.imageViews.size(), swapchain);
  }

  this->setupPostProcessingPipeline();
  this->createDefferedDescriptorPool();
  this->createDefferedDescriptorSets();

  this->setupCommandBuffers();
  this->recordCommandBuffers();

  //state.graphicsQueueMutex.unlock();

}

void Viewport::setupCommandBuffers() {

  commandBuffers.resize(swapchain.framebuffers.size());

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = state.graphicsCommandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = swapchain.framebuffers.size();

  if (vkAllocateCommandBuffers(state.device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
    throw dbg::trace_exception("Unable to allocate command buffer");

}

vkutil::VulkanState & Viewport::getState() {
  return state;
}


void Viewport::recordSingleBuffer(VkCommandBuffer & buffer, unsigned int frameIndex) {

  vkResetCommandBuffer(buffer, 0);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  beginInfo.pInheritanceInfo = nullptr;

  if (vkBeginCommandBuffer(buffer, &beginInfo) != VK_SUCCESS)
    throw dbg::trace_exception("Unable to start recording to command buffer");

  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = swapchain.framebuffers[frameIndex];
  renderPassInfo.renderArea.offset = {0,0};
  renderPassInfo.renderArea.extent = swapchain.extent;

  std::vector<VkClearValue> clearValues(5 + ppEffects.size());
  clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
  clearValues[1].depthStencil = {1.0f, 0};
  clearValues[2].color = {0.0f, 0.0f, 0.0f};
  clearValues[3].color = {0.0f, 0.0f, 0.0f};
  clearValues[4].color = {0.0f, 0.0f, 0.0f};

  for (unsigned int i = 0; i < ppEffects.size(); ++i) {
    clearValues[i + 5].color = {0.0f, 0.0f, 0.0f};
  }
  
  renderPassInfo.clearValueCount = clearValues.size();
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);


  if (bufferManager) {
    VkCommandBuffer secBuffer = bufferManager->getBufferForRender(frameIndex);
    //lout << "Submitting buffer " << secBuffer << " to " << buffer << " : " << frameIndex << std::endl;
    //bufferManager->printAttachedBuffers();
    if (secBuffer)
      vkCmdExecuteCommands(buffer, 1, &secBuffer);
  }

  vkCmdNextSubpass(buffer, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, defferedPipeline);

  ppBufferModel->bindForRender(buffer);

  vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, defferedPipelineLayout, 0, 1, &defferedDescSets[frameIndex], 0, nullptr);

  vkCmdDrawIndexed(buffer, ppBufferModel->getIndexCount(), 1, 0, 0, 0);

  for (std::shared_ptr<PPEffect> e : ppEffects) {
    vkCmdNextSubpass(buffer, VK_SUBPASS_CONTENTS_INLINE);

    e->bindForRender(buffer);
    ppBufferModel->bindForRender(buffer);
    e->bindDescriptorSets(buffer, frameIndex);
    
    vkCmdDrawIndexed(buffer, ppBufferModel->getIndexCount(), 1, 0, 0, 0);
    
  }

  vkCmdEndRenderPass(buffer);

  if (VkResult res = vkEndCommandBuffer(buffer))
    throw vkutil::vk_trace_exception("Unable to record command buffer", res);


}

void Viewport::recordCommandBuffers() {


  for (unsigned int i = 0; i < commandBuffers.size(); ++i) {

    recordSingleBuffer(commandBuffers[i], i);
  }

}

uint32_t Viewport::addLight(glm::vec4 pos, glm::vec4 color) {

  //if (lightIndex > 31) throw dbg::trace_exception("To many lights in viewport");
  if (lightIndex > VIEWPORT_MAX_LIGHT_COUNT-1) return 0xffffffff;

  this->lights.position[lightIndex] = pos;
  this->lights.color[lightIndex] = color;

  uint32_t index = lightIndex;

  this->lightIndex++;
  this->lights.activeCount = lightIndex;

  this->lightDataModified = 0xffffffff;

  return index;

}

void Viewport::updateLight(uint32_t index, glm::vec4 pos, glm::vec4 color) {

  if (index == 0xffffffff) return;

  lights.position[index] = pos;
  lights.color[index] = color;

  lightDataModified = 0xffffffff;

}

void Viewport::createSecondaryBuffers() {

  uint32_t bufferCount = swapchain.framebuffers.size() * 3;

  this->bufferManager = new ThreadedBufferManager(bufferCount, swapchain.framebuffers.size(), state);

}

void Viewport::renderIntoSecondary() {

  /// Get the next usable buffer.

  ThreadedBufferManager::BufferElement * bufferElem = bufferManager->getBufferForRecording();

  //lout << "Buffer " << bufferElem->buffer << " " << bufferElem->usageCount << std::endl;

  VkCommandBuffer buffer = bufferElem->buffer;

  /// Reset the buffer
  vkResetCommandBuffer(buffer, 0);

  VkCommandBufferInheritanceInfo inheritanceInfo = {};
  inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
  inheritanceInfo.renderPass = renderPass;
  inheritanceInfo.framebuffer = VK_NULL_HANDLE;
  inheritanceInfo.occlusionQueryEnable = VK_FALSE;

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
  beginInfo.pInheritanceInfo = &inheritanceInfo;

  if (vkBeginCommandBuffer(buffer, &beginInfo) != VK_SUCCESS)
    throw dbg::trace_exception("Unable to start recording to command buffer");

  /// Record the rendering commands
  /*for (auto it : renderElementsByShader) {

    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, it.first->getPipeline());
    vkCmdPushConstants(buffer, it.first->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, 32 * sizeof(float), &ubo);

    for (std::shared_ptr<RenderElement> relem : it.second) {

    relem->renderShaderless(buffer, frameIndex);

    }

    }*/

  for (std::shared_ptr<RenderElement> relem : renderElements) {
    relem->render(buffer, frameIndex);
  }

  //std::cout << "Ending buffer " << buffer << std::endl;
  if (VkResult res = vkEndCommandBuffer(buffer))
    throw vkutil::vk_trace_exception("Unable to end command buffer", res);

  // std::cout << "Submitting " << buffer << std::endl;
  /// Submit the resulting buffer as the current state
  bufferManager->setActiveBuffer(bufferElem);


}

ThreadedBufferManager::ThreadedBufferManager() {

}

ThreadedBufferManager::ThreadedBufferManager(unsigned int bufferCount, unsigned int frameCount, vkutil::VulkanState & state) {

  bufferPool = vkutil::createSecondaryCommandPool(state.physicalDevice, state.device, state.surface);

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandBufferCount = bufferCount;
  allocInfo.commandPool = bufferPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;

  std::vector<VkCommandBuffer> commandBuffers(bufferCount);
  buffers.resize(bufferCount);

  if (VkResult res = vkAllocateCommandBuffers(state.device, &allocInfo, commandBuffers.data()))
    throw vkutil::vk_trace_exception("Failed to allocate command buffers", res);

  useableBuffers = std::queue<BufferElement *>();

  uint32_t i = 0;
  for (VkCommandBuffer buffer : commandBuffers) {
    lout << "Queueing " << buffer << std::endl;
    //useableBuffers.push(buffer);

    BufferElement elem;
    elem.buffer = buffer;
    elem.usageCount = 0;
    buffers[i] = elem;

    useableBuffers.push(&buffers[i]);

    i++;

  }

  activeBuffer = nullptr;
  nextBuffer = nullptr;

  attachedBuffers.resize(frameCount);
  for (unsigned int i = 0; i < frameCount; ++i) {
    attachedBuffers[i] = nullptr;
  }

}

VkCommandBuffer ThreadedBufferManager::getBufferForRender(uint32_t frameIndex) {

  std::unique_lock<std::mutex> ulock(lock);

  if (!activeBuffer) {
    //lout << "No active buffer set" << std::endl;
    if (nextBuffer) {
      activeBuffer = nextBuffer;
      //return nextBuffer->buffer;
    } else {
      return VK_NULL_HANDLE;
    }
  }

  //std::cout << "Next: " << nextBuffer->buffer << " active: " << activeBuffer->buffer << std::endl;
  
  if (nextBuffer != activeBuffer) {
    //std::cout << "Active buffer usage count " << activeBuffer->usageCount << std::endl;
    BufferElement * tmp = activeBuffer;
    activeBuffer = nextBuffer;
    if (!tmp->usageCount) {
      pushBufferToUseable(tmp);
    }
  }

  if (attachedBuffers[frameIndex]) {
    releaseRenderBuffer(frameIndex, true);
  }

  if (attachedBuffers[frameIndex] != activeBuffer) {
    attachedBuffers[frameIndex] = activeBuffer;
    activeBuffer->usageCount++;
  }
  //lout << "Getting buffer for render " << frameIndex << " " << this->activeBuffer->buffer << std::endl;
  
  return this->activeBuffer->buffer;
}

void ThreadedBufferManager::releaseRenderBuffer(uint32_t frameIndex, bool internal) {

  if (!internal)
    lock.lock();

  if (frameIndex >= attachedBuffers.size())
    throw dbg::trace_exception(std::string("Frame index to high: ").append(std::to_string(frameIndex)).append(" >= ").append(std::to_string(attachedBuffers.size())));
  
  BufferElement * buffer = attachedBuffers[frameIndex];

  if (!buffer) {
    //lout << "Released buffer is NULL" << std::endl;
    if (!internal)
      lock.unlock();
    return;
  }

  /*for (BufferElement & e : buffers) {
    lout << "Buffer " << e.buffer << " " << e.usageCount << std::endl;
    }*/

  buffer->usageCount--;

  if (!buffer->usageCount && activeBuffer->buffer != buffer->buffer) {
    pushBufferToUseable(buffer);
    //lout << "Released " << buffer->buffer << " : " << buffer->usageCount << " frameIndex " << frameIndex << " queue: " << useableBuffers.size() << " active: "<< activeBuffer->buffer  << std::endl;
  }
  
  attachedBuffers[frameIndex] = nullptr;

  if (!internal)
    lock.unlock();
  
  //var.notify_all();

}

ThreadedBufferManager::BufferElement * ThreadedBufferManager::getBufferForRecording() {
  //lout << "Getting buffer for recording" << std::endl;
  std::unique_lock<std::mutex> ulock(lock);
  //lout << "Getting buffer for recording" << std::endl;

  if (useableBuffers.empty()) {

    //std::cout << "Waiting for buffer to be unused" << std::endl;
    
    //var.wait(ulock);
    //lout << "Attached Buffers: " << attachedBuffers.size() << std::endl;
    for (BufferElement & e : buffers) {
      lout << "Buffer " << e.buffer << " : " << e.usageCount << std::endl;
    }

    throw dbg::trace_exception("Empty buffer queue for recording");
  }


  BufferElement * buffer = useableBuffers.front();
  if (buffer->usageCount) {
    //lerr << "Buffer found in use: " << buffer->buffer << " : " << buffer->usageCount << std::endl;
    //lerr << "Active: " << activeBuffer->buffer << std::endl;
    throw dbg::trace_exception(std::string("attached buffer in queue: ").append(std::to_string(buffer->usageCount)));
  }
  useableBuffers.pop();

  //lout << "Returning " << buffer->buffer << " : " << buffer->usageCount << " for recording" << std::endl;

  return buffer;
}

void ThreadedBufferManager::setActiveBuffer(BufferElement * buffer) {

  std::unique_lock<std::mutex> ulock(lock);
  
  if (nextBuffer && nextBuffer->usageCount == 0 && nextBuffer != activeBuffer) {
    pushBufferToUseable(nextBuffer);
  }

  this->nextBuffer = buffer;

}

void ThreadedBufferManager::pushBufferToUseable(BufferElement * buffer) {

  if (buffer->usageCount)
    throw dbg::trace_exception(std::string("Pushing buffer with usage count != 0 : ").append(std::to_string(buffer->usageCount)));
  
  if (activeBuffer == buffer)
    throw dbg::trace_exception("Pushing activeBuffer into queue");

  useableBuffers.push(buffer);
  
}

void ThreadedBufferManager::printAttachedBuffers() {

  for (uint32_t i = 0; i < attachedBuffers.size(); ++i) {

    lout << "[" << i << "]: ";
    if (attachedBuffers[i])
      lout << attachedBuffers[i]->buffer << " " << attachedBuffers[i]->usageCount << std::endl;
    else
      lout << "NULL" << std::endl;
    
  }
  
}
