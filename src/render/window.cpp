#include "window.h"

#include "util/vkutil.h"

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

bool isDeviceSuitable(VkPhysicalDevice & device, VkSurfaceKHR & surface) {

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    vkutil::QueueFamilyIndices indices = vkutil::findQueueFamilies(device, surface);
    bool extensions = vkutil::checkDeviceExtensionSupport(device, deviceExtensions);

    bool swapChainOK = false;

    if (extensions) {
        vkutil::SwapChainSupportDetails details = vkutil::querySwapChainSupport(device, surface);
        swapChainOK = !details.formats.empty() && !details.presentModes.empty();
    }

    return indices.isComplete() && extensions && swapChainOK && supportedFeatures.samplerAnisotropy;
}

Window::Window() {

    state.glfwWindow = vkutil::createWindow(1280, 720, this);

    //std::vector<const char *> validationLayers(0);
    state.instance = vkutil::createInstance(validationLayers);
    vkutil::setupDebugMessenger(state.instance, validationLayers.size());

    state.surface = vkutil::createSurface(state.instance, state.glfwWindow);
    state.physicalDevice = vkutil::pickPhysicalDevice(state.instance, [&] (VkPhysicalDevice & d) -> bool {
        return isDeviceSuitable(d, state.surface);
    });

    state.device = vkutil::createLogicalDevice(state.physicalDevice, state.surface, &state.graphicsQueue, &state.presentQueue, &state.transferQueue, deviceExtensions);

    state.vmaAllocator = vkutil::createAllocator(state.device, state.physicalDevice);

    state.graphicsCommandPool = vkutil::createGraphicsCommandPool(state.physicalDevice, state.device, state.surface);
    state.transferCommandPool = vkutil::createTransferCommandPool(state.physicalDevice, state.device, state.surface);

}

Window::~Window() {

    /*vmaDestroyAllocator(state.vmaAllocator);

    vkutil::destroyWindow(state.glfwWindow);*/

}

VkPhysicalDevice & Window::getPhysicalDevice() {
    return state.physicalDevice;
}

VkInstance & Window::getInstance() {
    return state.instance;
}

VkSurfaceKHR & Window::getSurface() {
    return state.surface;
}

VkQueue & Window::getGraphicsQueue() {
    return state.graphicsQueue;
}

VkQueue & Window::getPresentQueue() {
    return state.presentQueue;
}

VkDevice & Window::getDevice() {
    return state.device;
}

VmaAllocator & Window::getAllocator() {
    return state.vmaAllocator;
}

GLFWwindow * Window::getGlfwWindow() {
    return state.glfwWindow;
}

VkCommandPool & Window::getCommandPool() {
    return state.graphicsCommandPool;
}

vkutil::VulkanState & Window::getState() {
    return state;
}
