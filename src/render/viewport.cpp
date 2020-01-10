#include "viewport.h"

Viewport::Viewport(std::shared_ptr<Window> window) :
    //physicalDevice(window->getPhysicalDevice()),
    //instance(window->getInstance()),
    //surface(window->getSurface()),
    graphicsQueue(window->getGraphicsQueue()),
    //presentQueue(window->getPresentQueue()),
    device(window->getDevice()),
    vmaAllocator(window->getAllocator()),
    commandPool(window->getCommandPool())
{

    swapchain = vkutil::createSwapchain(window->getPhysicalDevice(), device, window->getSurface(), window->getGlfwWindow());
    setupRenderPass();

    /** Creating depth resources **/
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    vkutil::createImage(vmaAllocator, device, swapchain.extent.width, swapchain.extent.height, 1, 1, depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    depthImageView = vkutil::createImageView(device, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    vkutil::transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, commandPool, device, graphicsQueue);

}

Viewport::~Viewport()
{
    //dtor
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

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        throw std::runtime_error("Unable to create renderpass");

}
