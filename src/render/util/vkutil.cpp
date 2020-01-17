#include "vkutil.h"

#include <string.h>
#include <stdexcept>
#include <iostream>
#include <limits>
#include <set>
#include <fstream>

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

using namespace vkutil;

VkDebugUtilsMessengerEXT debugMessenger;

GLFWwindow * vkutil::createWindow(unsigned int width, unsigned int height, void * userData) {

    GLFWwindow * window;

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(width, height, "Window", nullptr, nullptr);
    glfwSetWindowUserPointer(window, userData);

    return window;

}

void vkutil::destroyWindow(GLFWwindow * window) {

    glfwDestroyWindow(window);
    glfwTerminate();

}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

#define THROW_ON_WARN
#define BACKTRACE_DEPTH 256

void * backtrace_array[BACKTRACE_DEPTH];

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        size_t s;

        s = backtrace(backtrace_array, BACKTRACE_DEPTH);
        backtrace_symbols_fd(backtrace_array, s, STDERR_FILENO);

        throw std::runtime_error(pCallbackData->pMessage);
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        #ifdef THROW_ON_WARN
        size_t s;

        s = backtrace(backtrace_array, BACKTRACE_DEPTH);
        backtrace_symbols_fd(backtrace_array, s, STDERR_FILENO);
        throw std::runtime_error(pCallbackData->pMessage);
        #else
        std::cerr << "WARNING : " << pCallbackData->pMessage << std::endl;
        #endif
    } else {
        std::cout << "DEBUG   : " << pCallbackData->pMessage << std::endl;
    }
    //exit(1);
    return VK_FALSE;

}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT & createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

void vkutil::setupDebugMessenger(const VkInstance & instance, bool validationLayers) {
    if (!validationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

bool checkValidationLayerSupport(std::vector<const char *> validationLayers) {

    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> properties(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, properties.data());

    for (const char * name : validationLayers) {

        bool layerFound = false;
        for (auto layerProperties : properties) {

            if (!strcmp(name, layerProperties.layerName)) {
                layerFound = true;
                break;
            }

        }

        if (!layerFound) {
            throw std::runtime_error(std::string("Missing layer: ").append(name));
            return false;
        }

    }

    return true;

}

VkInstance vkutil::createInstance(std::vector<const char *> validationLayers) {

    if (validationLayers.size() && !checkValidationLayerSupport(validationLayers))
        throw std::runtime_error("Validation layers not found but requested!");

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "VkEngine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "VkEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    const char ** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char *> requiredExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (validationLayers.size()) {
        requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    createInfo.enabledExtensionCount = requiredExtensions.size();
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    createInfo.enabledLayerCount = validationLayers.size();

    if (validationLayers.size()) {
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.ppEnabledLayerNames = nullptr;
    }

    VkInstance instance;

    if (vkCreateInstance(&createInfo, nullptr, &instance)) {
        throw std::runtime_error("Unable to create Instance");
    }

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    std::cout << "available extensions:" << std::endl;
    for (int i = 0; i < extensions.size(); ++i) {

        std::cout << "\t" << extensions[i].extensionName << std::endl;

    }

    return instance;

}

VkSurfaceKHR vkutil::createSurface(VkInstance instance, GLFWwindow * window) {

    VkSurfaceKHR surface;

    if (glfwCreateWindowSurface(instance, window, nullptr, &surface))
        throw std::runtime_error("Could not create surface");

    return surface;

}

vkutil::QueueFamilyIndices vkutil::findQueueFamilies(const VkPhysicalDevice & device, const VkSurfaceKHR & surface) {

    QueueFamilyIndices indices;

    uint32_t familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);
    std::vector<VkQueueFamilyProperties> families(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, families.data());

    int i = 0;

    for (VkQueueFamilyProperties p : families) {

        if (p.queueCount > 0 && p.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphicsFamily = i;

        if (p.queueCount > 0 && p.queueFlags & VK_QUEUE_TRANSFER_BIT)
            indices.transferFamily = i;

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (p.queueCount > 0 && presentSupport)
            indices.presentFamily = i;

        if (indices.isComplete())
            break;

        ++i;

    }

    return indices;

}

SwapChainSupportDetails vkutil::querySwapChainSupport(const VkPhysicalDevice & device, const VkSurfaceKHR & surface) {

    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    details.formats = std::vector<VkSurfaceFormatKHR>(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());

    uint32_t modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modeCount, nullptr);
    details.presentModes = std::vector<VkPresentModeKHR>(modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modeCount, details.presentModes.data());

    return details;

}

bool vkutil::checkDeviceExtensionSupport(VkPhysicalDevice pdevice, const std::vector<const char*> deviceExtensions) {

    uint32_t extensionCount=0;
    vkEnumerateDeviceExtensionProperties(pdevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> properties(extensionCount);
    vkEnumerateDeviceExtensionProperties(pdevice, nullptr, &extensionCount, properties.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto & e : properties) {
        requiredExtensions.erase(std::string(e.extensionName));
    }

    return requiredExtensions.empty();

}

VkPhysicalDevice vkutil::pickPhysicalDevice(VkInstance instance, std::function<bool(VkPhysicalDevice &)> isDeviceSuitable) {

    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (!deviceCount) throw std::runtime_error("No GPU found");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    VkPhysicalDevice device = VK_NULL_HANDLE;

    for (VkPhysicalDevice d : devices) {

        if (isDeviceSuitable(d)) {
            device = d;
            break;
        }

    }

    if (device == VK_NULL_HANDLE)
        throw std::runtime_error("unable to find physical device");

    return device;

}

VkDevice vkutil::createLogicalDevice(VkPhysicalDevice & pDevice, VkSurfaceKHR & surface, VkQueue * gQueue, VkQueue * pQueue, VkQueue * tQueue, const std::vector<const char*> deviceExtensions) {

    QueueFamilyIndices indices = findQueueFamilies(pDevice, surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> uniqueFamilies = {indices.graphicsFamily, indices.presentFamily, indices.transferFamily};

    for (int family : uniqueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = family;
        queueCreateInfo.queueCount = 1;
        float priority = 1.0;
        queueCreateInfo.pQueuePriorities = &priority;

        queueCreateInfos.push_back(queueCreateInfo);

    }

    VkDevice device;

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = queueCreateInfos.size();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.enabledLayerCount = 0;

    if (vkCreateDevice(pDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        throw std::runtime_error("Unable to create logical device.");

    vkGetDeviceQueue(device, indices.graphicsFamily, 0, gQueue);
    vkGetDeviceQueue(device, indices.presentFamily, 0, pQueue);
    vkGetDeviceQueue(device, indices.transferFamily, 0, tQueue);

    return device;

}

VmaAllocator vkutil::createAllocator(VkDevice & device, VkPhysicalDevice & physicalDevice) {

    VmaAllocator a;

    VmaAllocatorCreateInfo createInfo = {}; /// <- very important to initialize to 0.
    createInfo.device = device;
    createInfo.physicalDevice = physicalDevice;

    vmaCreateAllocator(&createInfo, &a);

    return a;

}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> & formats) {

    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
        return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }

    for (const VkSurfaceFormatKHR & f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return f;
    }

    return formats[0];

}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> & modes) {

    VkPresentModeKHR best = VK_PRESENT_MODE_FIFO_KHR;

    for (const VkPresentModeKHR m : modes) {
        if (m == VK_PRESENT_MODE_MAILBOX_KHR)
            return m;
        if(m == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            best = m;
        }
    }

    return best;

}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities, GLFWwindow * window) {

    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D newExtent = {width, height};

    newExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, newExtent.width));
    newExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, newExtent.height));

    return newExtent;

}

SwapChain vkutil::createSwapchain(const VkPhysicalDevice & physicalDevice, const VkDevice & device, const VkSurfaceKHR & surface, GLFWwindow * window) {

    SwapChainSupportDetails details = querySwapChainSupport(physicalDevice, surface);
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(details.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(details.presentModes);
    VkExtent2D ext = chooseSwapExtent(details.capabilities, window);

    uint32_t imageCount = details.capabilities.minImageCount + 1;

    if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount)
        imageCount = details.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;

    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = ext;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

    if (indices.presentFamily != indices.graphicsFamily) {

        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;

    } else {

        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;

    }

    createInfo.preTransform = details.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainFormat;
    VkExtent2D swapChainExtent;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create swapchain");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainFormat = surfaceFormat.format;
    swapChainExtent = ext;

    SwapChain chain;
    chain.chain = swapChain;
    chain.extent = swapChainExtent;
    chain.format = swapChainFormat;
    chain.images = swapChainImages;

    return chain;

}

VkCommandPool vkutil::createGraphicsCommandPool(const VkPhysicalDevice & physicalDevice, const VkDevice & device, const VkSurfaceKHR & surface) {

    VkCommandPool commandPool;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.queueFamilyIndex = indices.graphicsFamily;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device, &createInfo, nullptr, &commandPool) != VK_SUCCESS)
        throw std::runtime_error("Unable to create command pool");

    return commandPool;

}

VkCommandPool vkutil::createTransferCommandPool(const VkPhysicalDevice & physicalDevice, const VkDevice & device, const VkSurfaceKHR & surface) {

    VkCommandPool commandPool;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.queueFamilyIndex = indices.transferFamily;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device, &createInfo, nullptr, &commandPool) != VK_SUCCESS)
        throw std::runtime_error("Unable to create command pool");

    return commandPool;

}

void vkutil::createImage(const VmaAllocator & allocator, VkDevice device, int width, int height, int depth, int mipLevels, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlagBits memProps, VkImage & image, VmaAllocation & memory) {

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = depth;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage; //
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.requiredFlags = memProps;

    int retVal;

    if ((retVal = vmaCreateImage(allocator, &imageInfo, &allocInfo, &image, &memory, nullptr)) != VK_SUCCESS)
        throw std::runtime_error(std::string("Could not create image ").append(std::to_string(retVal)));

}

VkImageView vkutil::createImageView(const VkDevice & device, const VkImage & image, VkFormat format, VkImageAspectFlags aspect, int mipLevels) {

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;

    viewInfo.subresourceRange.aspectMask = aspect;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
        throw std::runtime_error("Unable to create image view");

    return imageView;

}

bool vkutil::hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void vkutil::transitionImageLayout(const VkImage & image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, int mipLevels, const VkCommandPool & commandPool, const VkDevice & device, const VkQueue & q) {

    VkCommandBuffer commandBuffer = beginSingleCommand(commandPool, device);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (vkutil::hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleCommand(commandBuffer, commandPool, device, q);

}

VkCommandBuffer vkutil::beginSingleCommand(const VulkanState & state) {
    return beginSingleCommand(state.graphicsCommandPool, state.device);
}

VkCommandBuffer vkutil::beginSingleCommand(const VkCommandPool & commandPool, const VkDevice & device) {

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;

}

void vkutil::endSingleCommand(VkCommandBuffer & commandBuffer, const VulkanState & state) {
    endSingleCommand(commandBuffer, state.graphicsCommandPool, state.device, state.graphicsQueue);
}

void vkutil::endSingleCommand(VkCommandBuffer & commandBuffer, const VkCommandPool & commandPool, const VkDevice & device, const VkQueue & q) {

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(q, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(q);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

}

std::vector<uint8_t> readFile(const std::string & fname) {

    std::ifstream file(fname, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error(std::string("Unable to open file ").append(fname));
    }

    size_t fileSize = file.tellg();
    std::vector<uint8_t> data(fileSize);

    file.seekg(0);
    file.read((char*)data.data(), fileSize);
    file.close();

    return data;

}


VkShaderModule vkutil::createShaderModule(const std::vector<uint8_t> & code, const VkDevice & device) {

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    VkShaderModule module;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &module) != VK_SUCCESS)
        throw std::runtime_error("Unable to create shader module");

    return module;

}

std::vector<VkImageView> vkutil::createSwapchainImageViews(const std::vector<VkImage> & swapChainImages, VkFormat swapChainFormat, const VkDevice & device) {

    std::vector<VkImageView> swapChainImageViews(swapChainImages.size());

    for (int i = 0; i < swapChainImages.size(); ++i) {

        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
            throw std::runtime_error("Could not create image view for swapChain");

    }

    return swapChainImageViews;

}

void vkutil::copyBuffer(VkBuffer & src, VkBuffer & dst, VkDeviceSize & size, const VkCommandPool & commandPool, const VkDevice & device, const VkQueue & q) {

    VkCommandBuffer commandBuffer = beginSingleCommand(commandPool, device);

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;

    vkCmdCopyBuffer(commandBuffer,src, dst, 1, &copyRegion);

    endSingleCommand(commandBuffer, commandPool, device, q);

}

VkDescriptorSetLayout vkutil::createDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding> & bindings, const VkDevice & device) {

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.pBindings = bindings.data();
    layoutInfo.bindingCount = bindings.size();

    VkDescriptorSetLayout descriptorSetLayout;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
        throw std::runtime_error("could not create descriptor set layout.");

    return descriptorSetLayout;

}

VkPipeline vkutil::createGraphicsPipeline(const VulkanState & state, const VkRenderPass & renderPass, std::vector<ShaderInputDescription>& shaders, VertexInputDescriptions & descs, VkDescriptorSetLayout & descriptorSetLayout, VkPipelineLayout & retLayout, VkExtent2D swapChainExtent) {

    /** shaders **/

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


    vertexInputInfo.vertexBindingDescriptionCount = descs.binding.size();
    vertexInputInfo.pVertexBindingDescriptions = descs.binding.data();
    vertexInputInfo.vertexAttributeDescriptionCount = descs.attributes.size();
    vertexInputInfo.pVertexAttributeDescriptions = descs.attributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    /* Viewport */

    VkViewport viewport = {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = swapChainExtent.width;
    viewport.height = swapChainExtent.height;
    viewport.minDepth = 0.0;
    viewport.maxDepth = 1.0;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;

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
    //rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
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

    std::array<VkPipelineColorBlendAttachmentState, 3> cbAttachments = {colorBlendAttachment, colorBlendAttachment, colorBlendAttachment};

    // global configuration
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = cbAttachments.size();
    colorBlending.pAttachments = cbAttachments.data();
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
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; ///Get from Shader object
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(state.device, &pipelineLayoutInfo, nullptr, &retLayout) != VK_SUCCESS)
        throw std::runtime_error("Unable to create pipeline layout");


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
    pipelineInfo.layout = retLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VkPipeline graphicsPipeline;

    if (vkCreateGraphicsPipelines(state.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
        throw std::runtime_error("Unable to create pipeline");

    return graphicsPipeline;

}
