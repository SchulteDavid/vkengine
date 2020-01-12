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

    glfwWindow = vkutil::createWindow(1280, 720, this);

    //std::vector<const char *> validationLayers(0);
    instance = vkutil::createInstance(validationLayers);
    //vkutil::setupDebugMessenger(instance, validationLayers.size());

    surface = vkutil::createSurface(instance, glfwWindow);
    physicalDevice = vkutil::pickPhysicalDevice(instance, [&] (VkPhysicalDevice & d) -> bool {
        return isDeviceSuitable(d, surface);
    });

    device = vkutil::createLogicalDevice(physicalDevice, surface, &graphicsQueue, &presentQueue, deviceExtensions);

    vmaAllocator = vkutil::createAllocator(device, physicalDevice);

    commandPool = vkutil::createCommandPool(physicalDevice, device, surface);

}

Window::~Window() {

    vmaDestroyAllocator(vmaAllocator);

    vkutil::destroyWindow(glfwWindow);

}

VkPhysicalDevice & Window::getPhysicalDevice() {
    return physicalDevice;
}

VkInstance & Window::getInstance() {
    return instance;
}

VkSurfaceKHR & Window::getSurface() {
    return surface;
}

VkQueue & Window::getGraphicsQueue() {
    return graphicsQueue;
}

VkQueue & Window::getPresentQueue() {
    return presentQueue;
}

VkDevice & Window::getDevice() {
    return device;
}

VmaAllocator & Window::getAllocator() {
    return vmaAllocator;
}

GLFWwindow * Window::getGlfwWindow() {
    return glfwWindow;
}

VkCommandPool & Window::getCommandPool() {
    return commandPool;
}
