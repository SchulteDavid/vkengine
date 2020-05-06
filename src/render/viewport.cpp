#include "viewport.h"

#include <iostream>
#include <chrono>

#define MAX_FRAMES_IN_FLIGHT 4

#include "util/debug/trace_exception.h"
#include "util/debug/logger.h"

struct Viewport::CameraData {

    alignas(16) glm::vec3 position;

};

std::vector<Model::Vertex> viewModelData = {

    {glm::vec3(-1, -1, 0), glm::vec3(0, 0, 1), glm::vec3(1, 0, 0), glm::vec2(0,0), 0},
    {glm::vec3(1, -1, 0),  glm::vec3(0, 0, 1), glm::vec3(1, 0, 0), glm::vec2(1,0), 0},
    {glm::vec3(1, 1, 0),   glm::vec3(0, 0, 1), glm::vec3(1, 0, 0), glm::vec2(1,1), 0},
    {glm::vec3(-1, 1, 0),  glm::vec3(0, 0, 1), glm::vec3(1, 0, 0), glm::vec2(0,1), 0},

};

std::vector<uint16_t> viewModelIndices = {
    0, 1, 2,
    0, 2, 3,
};

Viewport::Viewport(std::shared_ptr<Window> window, Camera * camera) : state(window->getState()) {

    this->camera = camera;
    this->lightIndex = 0;
    //camera->move(0,0,1);

    this->frameIndex = 0;

    logger(std::cout) << "Creating swapchain" << std::endl;

    vkutil::SwapChain tmpChain = vkutil::createSwapchain(window->getPhysicalDevice(), state.device, window->getSurface(), window->getGlfwWindow());

    swapchain.chain = tmpChain.chain;
    swapchain.extent = tmpChain.extent;
    swapchain.format = tmpChain.format;
    swapchain.images = tmpChain.images;

    logger(std::cout) << "setting up render pass" << std::endl;

    setupRenderPass();

    /** Creating depth resources **/
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    vkutil::createImage(state.vmaAllocator, state.device, swapchain.extent.width, swapchain.extent.height, 1, 1, depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    depthImageView = vkutil::createImageView(state.device, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    vkutil::transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, state.graphicsCommandPool, state.device, state.graphicsQueue);

    swapchain.imageViews = vkutil::createSwapchainImageViews(swapchain.images, swapchain.format, state.device);

    createPpDescriptorSetLayout();
    createPPObjects();
    setupPostProcessingPipeline();

    setupFramebuffers();
    createPpDescriptorPool();
    createPPDescriptorSets();

    setupCommandBuffers();

    createTransferCommandBuffer();

    createSyncObjects();

    //ppBufferModel = std::shared_ptr<Model>(Model::loadFromFile(state, "resources/models/quad.ply"));
    ppBufferModel = std::shared_ptr<Model>(new Model(state, viewModelData, viewModelIndices));

    ppBufferModel->uploadToGPU(state.device, state.graphicsCommandPool, state.graphicsQueue);

    recordCommandBuffers();

}

Viewport::~Viewport() {
    for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        vkWaitForFences(state.device, 1, &inFlightFences[i], VK_TRUE, std::numeric_limits<uint64_t>::max());
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

        std::cout << "Creating fence for frame " << i << std::endl;
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

        //std::cout << "Waiting for transfer fences" << std::endl;
        vkWaitForFences(state.device, 1, &transferFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
        vkResetFences(state.device, 1, &transferFence);

        //std::cout << "Recording transfer" << std::endl;
        this->recordTransfer(transferCmdBuffer);

        VkSubmitInfo transferSubmit = {};
        transferSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        transferSubmit.commandBufferCount = 1;
        transferSubmit.pCommandBuffers = &transferCmdBuffer;
        transferSubmit.signalSemaphoreCount = 0;
        transferSubmit.waitSemaphoreCount = 0;

        state.graphicsQueue.lock();
        vkQueueSubmit(state.transferQueue.q, 1, &transferSubmit, transferFence);
        state.graphicsQueue.unlock();

    }

}

void Viewport::drawFrame(bool updateElements) {

    static auto startRenderTime = std::chrono::high_resolution_clock::now();

    if (updateElements)
        prepareRenderElements();

    vkWaitForFences(state.device, 1, &inFlightFences[frameIndex], VK_TRUE, std::numeric_limits<uint64_t>::max());


    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(state.device, swapchain.chain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex);

    if (!this->destroyableSwapchains.empty()) {

        SwapchainInfo i = destroyableSwapchains.front();

        //vkDestroySwapchainKHR(state.device, i.chain, nullptr);

        //destroyableSwapchains.pop();

    }

    switch(result) {

    case VK_SUBOPTIMAL_KHR:
    case VK_ERROR_OUT_OF_DATE_KHR:
        //state.graphicsQueue.lock();
        //vkQueueWaitIdle(state.graphicsQueue.q);
        //state.graphicsQueue.unlock();
        std::cout << "destroying swapchain" << std::endl;
        destroySwapChain();
        std::cout << "Swap chains destroyed" << std::endl;
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
    if (vkQueueSubmit(state.graphicsQueue.q, 1, &submitInfo, inFlightFences[frameIndex]) != VK_SUCCESS)
        throw dbg::trace_exception("Unable to submit command buffer");
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

    state.presentQueue.lock();
    vkQueuePresentKHR(state.presentQueue.q, &presentInfo);
    state.presentQueue.unlock();

    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;

    if (!frameIndex) {
        double duration = std::chrono::duration<double, std::chrono::milliseconds::period>(std::chrono::high_resolution_clock::now() - startRenderTime).count();
        logger(std::cout) << "Frame time: " << duration << "ms => fps: " << (1000.0 / duration) << std::endl;
    }

    startRenderTime = std::chrono::high_resolution_clock::now();

}

Camera * Viewport::getCamera() {
    return camera;
}

void Viewport::updateUniformBuffer(uint32_t imageIndex) {

    //static auto startTime = std::chrono::high_resolution_clock::now();
    //auto currentTime = std::chrono::high_resolution_clock::now();

    //float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo;

    ubo.view = this->camera->getView();//glm::lookAt(glm::vec3(0.0, -4.0f, 2.0f), glm::vec3(0,0,0), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = this->camera->getProjection();//glm::perspective(glm::radians(45.0f), swapChain.extent.width / (float) swapChain.extent.height, 0.1f, 1000.0f);

    for (unsigned int i = 0; i < renderElements.size(); ++i) {

        renderElements[i]->updateUniformBuffer(ubo, imageIndex);

    }

    //std::cout << "Copying light data" << std::endl;

    void * data;
    vmaMapMemory(state.vmaAllocator, ppLightBuffersMemory[imageIndex], &data);
    memcpy(data, &lights, sizeof(LightData));
    vmaUnmapMemory(state.vmaAllocator, ppLightBuffersMemory[imageIndex]);

    vmaMapMemory(state.vmaAllocator, ppCameraBuffersMemory[imageIndex], &data);
    CameraData * camData = (CameraData *) data;
    camData->position = camera->position;
    vmaUnmapMemory(state.vmaAllocator, ppCameraBuffersMemory[imageIndex]);

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
    colorAttachmentRef.attachment = 0;
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

    std::array<VkAttachmentDescription, 5> attachments = {colorAttachment, depthAttachment, gAttachment, nAttachment, aAttachment};

    std::array<VkSubpassDescription, 2> subpasses = {subpass, subpass2};
    std::array<VkSubpassDependency, 2> dependencies = {dependency, dependency2};

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

void Viewport::createPPObjects() {

    vkutil::createImage(state.vmaAllocator, state.device, swapchain.extent.width, swapchain.extent.height, 1, 1, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gBufferImage, gBufferImageMemory);
    gBufferImageView = vkutil::createImageView(state.device, gBufferImage, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    vkutil::createImage(state.vmaAllocator, state.device, swapchain.extent.width, swapchain.extent.height, 1, 1, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, nBufferImage, nBufferImageMemory);
    nBufferImageView = vkutil::createImageView(state.device, nBufferImage, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    vkutil::createImage(state.vmaAllocator, state.device, swapchain.extent.width, swapchain.extent.height, 1, 1, swapchain.format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, aBufferImage, aBufferImageMemory);
    aBufferImageView = vkutil::createImageView(state.device, aBufferImage, swapchain.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    VkDeviceSize lightSize = sizeof(LightData);
    VkDeviceSize cameraSize = sizeof(CameraData);

    ppLightBuffers.resize(swapchain.images.size());
    ppLightBuffersMemory.resize(swapchain.images.size());

    ppCameraBuffers.resize(swapchain.images.size());
    ppCameraBuffersMemory.resize(swapchain.images.size());

    logger(std::cout) << "Images created" << std::endl;

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

            logger(std::cout) << "Creating light Buffer" << std::endl;

            vmaCreateBuffer(state.vmaAllocator, &stBufferCreateInfo, &stAllocCreateInfo, &ppLightBuffers[i], &ppLightBuffersMemory[i], &stagingBufferAllocInfo);
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

            logger(std::cout) << "Creating camera Buffer" << std::endl;

            vmaCreateBuffer(state.vmaAllocator, &stBufferCreateInfo, &stAllocCreateInfo, &ppCameraBuffers[i], &ppCameraBuffersMemory[i], &stagingBufferAllocInfo);
        }
        //StorageBuffer::createBuffer(vmaAllocator, cameraSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, ppCameraBuffers[i], ppCameraBuffersMemory[i]);

    }

}

void Viewport::destroyPPObjects() {

    vmaDestroyImage(state.vmaAllocator, gBufferImage, gBufferImageMemory);
    vkDestroyImageView(state.device, gBufferImageView, nullptr);

    vmaDestroyImage(state.vmaAllocator, nBufferImage, nBufferImageMemory);
    vkDestroyImageView(state.device, nBufferImageView, nullptr);

    vmaDestroyImage(state.vmaAllocator, aBufferImage, aBufferImageMemory);
    vkDestroyImageView(state.device, aBufferImageView, nullptr);

    for (unsigned int i = 0; i < swapchain.images.size(); ++i) {

        vmaDestroyBuffer(state.vmaAllocator, ppLightBuffers[i], ppLightBuffersMemory[i]);
        vmaDestroyBuffer(state.vmaAllocator, ppCameraBuffers[i], ppCameraBuffersMemory[i]);

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

    logger(std::cout) << "Creating ppPipeline" << std::endl;

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages(2);

    shaderStages[0].module = vkutil::createShaderModule(readFile("resources/shaders/id.vert.spirv"), state.device);
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].pName = "main";


    shaderStages[1].module = vkutil::createShaderModule(readFile("resources/shaders/pp.frag.spirv"), state.device);
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].pName = "main";

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
    pipelineLayoutInfo.pSetLayouts = &ppDescLayout; ///Get from Shader object
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(state.device, &pipelineLayoutInfo, nullptr, &ppPipelineLayout) != VK_SUCCESS)
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
    pipelineInfo.layout = ppPipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 1;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(state.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &ppPipeline) != VK_SUCCESS)
        throw dbg::trace_exception("Unable to create pipeline");

}

void Viewport::createPpDescriptorSetLayout() {

    std::array<VkDescriptorSetLayoutBinding, 5> bindings;
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

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.pBindings = bindings.data();
    layoutInfo.bindingCount = bindings.size();

    if (vkCreateDescriptorSetLayout(state.device, &layoutInfo, nullptr, &ppDescLayout) != VK_SUCCESS)
        throw dbg::trace_exception("could not create descriptor set layout.");

}

void Viewport::createPpDescriptorPool() {

    VkDescriptorPoolSize samplerSize = {};
    samplerSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    samplerSize.descriptorCount = swapchain.images.size();

    VkDescriptorPoolSize lightSize = {};
    lightSize.descriptorCount = swapchain.images.size();
    lightSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    VkDescriptorPoolSize cameraSize = {};
    cameraSize.descriptorCount = swapchain.images.size();
    cameraSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    VkDescriptorPoolSize sizes[] = {samplerSize, samplerSize, samplerSize, lightSize, cameraSize};

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 5;
    poolInfo.pPoolSizes = sizes;
    poolInfo.maxSets = swapchain.images.size();

    if (vkCreateDescriptorPool(state.device, &poolInfo, nullptr, &ppDescPool) != VK_SUCCESS)
        throw dbg::trace_exception("Unable to create descriptor pool");

}

void Viewport::createPPDescriptorSets() {

    std::vector<VkDescriptorSetLayout> layouts(swapchain.images.size(), ppDescLayout);
    this->ppDescSets.resize(swapchain.images.size());

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = ppDescPool;
    allocInfo.descriptorSetCount = swapchain.images.size();
    allocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(state.device, &allocInfo, ppDescSets.data()) != VK_SUCCESS)
        throw dbg::trace_exception("Unable to allocate descriptor sets");

    for (unsigned int i = 0; i < swapchain.images.size(); ++i) {

        std::array<VkWriteDescriptorSet, 5> descriptorWrites = {};

        VkDescriptorImageInfo gInfo;
        gInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        gInfo.imageView = gBufferImageView;
        gInfo.sampler = VK_NULL_HANDLE;

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = ppDescSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = nullptr;
        descriptorWrites[0].pImageInfo = &gInfo;
        descriptorWrites[0].pTexelBufferView = nullptr;

        VkDescriptorImageInfo nInfo;
        nInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        nInfo.imageView = nBufferImageView;
        nInfo.sampler = VK_NULL_HANDLE;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = ppDescSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = nullptr;
        descriptorWrites[1].pImageInfo = &nInfo;
        descriptorWrites[1].pTexelBufferView = nullptr;

        VkDescriptorImageInfo aInfo;
        aInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        aInfo.imageView = aBufferImageView;
        aInfo.sampler = VK_NULL_HANDLE;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = ppDescSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = nullptr;
        descriptorWrites[2].pImageInfo = &aInfo;
        descriptorWrites[2].pTexelBufferView = nullptr;

        VkDescriptorBufferInfo lightInfo = {};
        lightInfo.buffer = this->ppLightBuffers[i];
        lightInfo.offset = 0;
        lightInfo.range = sizeof(LightData);

        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstSet = ppDescSets[i];
        descriptorWrites[3].dstBinding = 3;
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[3].descriptorCount = 1;
        descriptorWrites[3].pBufferInfo = &lightInfo;
        descriptorWrites[3].pImageInfo = nullptr;
        descriptorWrites[3].pTexelBufferView = nullptr;

        VkDescriptorBufferInfo cameraInfo = {};
        cameraInfo.buffer = this->ppCameraBuffers[i];
        cameraInfo.offset = 0;
        cameraInfo.range = sizeof(CameraData);

        descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[4].dstSet = ppDescSets[i];
        descriptorWrites[4].dstBinding = 4;
        descriptorWrites[4].dstArrayElement = 0;
        descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[4].descriptorCount = 1;
        descriptorWrites[4].pBufferInfo = &cameraInfo;
        descriptorWrites[4].pImageInfo = nullptr;
        descriptorWrites[4].pTexelBufferView = nullptr;

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

    logger(std::cout) << "SwapChainSize " << swapchain.imageViews.size() << std::endl;

    for (unsigned int i = 0; i < swapchain.imageViews.size(); ++i) {

        std::array<VkImageView, 5> attachments = {swapchain.imageViews[i], depthImageView, gBufferImageView, nBufferImageView, aBufferImageView};

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

    destroyPPObjects();

    std::cout << "PP Objects destroyed" << std::endl;

    for (auto framebuffer : swapchain.framebuffers) {
        vkDestroyFramebuffer(state.device, framebuffer, nullptr);
    }

    std::cout << "Destroyed framebuffers" << std::endl;

    vkFreeCommandBuffers(state.device, state.graphicsCommandPool, commandBuffers.size(), commandBuffers.data());

    vkDestroyRenderPass(state.device, renderPass, nullptr);

    std::cout << "Destroyed renderpass" << std::endl;

    vkDestroyImageView(state.device, depthImageView, nullptr);
    vmaDestroyImage(state.vmaAllocator, depthImage, depthImageMemory);

    std::cout << "Destroyed depth image" << std::endl;

    for (const VkImageView & v : swapchain.imageViews) {
        vkDestroyImageView(state.device, v, nullptr);
    }

    for (const VkImage & i : swapchain.images) {
        vkDestroyImage(state.device, i, nullptr);
    }

    std::cout << "Destroyed images" << std::endl;

    destroyableSwapchains.push(swapchain);

    for (unsigned int i = 0; i < renderElements.size(); ++i) {
        this->renderElements[i]->destroyUniformBuffers(swapchain);
    }

    std::cout << "Destroyed uniform buffers" << std::endl;

}

void Viewport::recreateSwapChain() {

    std::cout << "Recreating swapchain" << std::endl;

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
    logger(std::cout) << "Creating depth image view" << std::endl;
    depthImageView = vkutil::createImageView(state.device, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

    vkutil::transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, state.graphicsCommandPool, state.device, state.graphicsQueue);

    logger(std::cout) << "Creating PP objects" << std::endl;
    createPPObjects();

    this->setupFramebuffers();

    logger(std::cout) << "Done creating Framebuffers" << std::endl;

    for (unsigned int i = 0; i < renderElements.size(); ++i) {
        renderElements[i]->recreateResources(renderPass, swapchain.imageViews.size(), swapchain);
    }

    this->setupPostProcessingPipeline();
    this->createPpDescriptorPool();
    this->createPPDescriptorSets();

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

void Viewport::recordCommandBuffers() {


    for (unsigned int i = 0; i < commandBuffers.size(); ++i) {

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
            throw dbg::trace_exception("Unable to start recording to command buffer");

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapchain.framebuffers[i];
        renderPassInfo.renderArea.offset = {0,0};
        renderPassInfo.renderArea.extent = swapchain.extent;

        std::array<VkClearValue, 5> clearValues;
        clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};
        clearValues[2].color = {0.0f, 0.0f, 0.0f};
        clearValues[3].color = {0.0f, 0.0f, 0.0f};
        clearValues[4].color = {0.0f, 0.0f, 0.0f};
        renderPassInfo.clearValueCount = clearValues.size();
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        /*for (unsigned int j = 0; j < renderElements.size(); ++j) {

            renderElements[j]->render(commandBuffers[i], i);

        }*/

        for (auto it : renderElementsByShader) {

            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, it.first->getPipeline());

            for (std::shared_ptr<RenderElement> relem : it.second) {

                relem->renderShaderless(commandBuffers[i], i);

            }

        }

        vkCmdNextSubpass(commandBuffers[i], VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, ppPipeline);

        ppBufferModel->bindForRender(commandBuffers[i]);

        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, ppPipelineLayout, 0, 1, &ppDescSets[i], 0, nullptr);

        vkCmdDrawIndexed(commandBuffers[i], ppBufferModel->getIndexCount(), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
            throw dbg::trace_exception("Unable to record command buffer");

    }

}

void Viewport::addLight(glm::vec4 pos, glm::vec4 color) {

    if (lightIndex > 31) throw dbg::trace_exception("To many lights in viewport");

    this->lights.position[lightIndex] = pos;
    this->lights.color[lightIndex] = color;

    this->lightIndex++;
    this->lights.activeCount = lightIndex;

}
