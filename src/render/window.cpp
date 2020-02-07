#include "window.h"

#include "util/vkutil.h"

#include <GLFW/glfw3.h>
#include <iostream>

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#if DEBUG
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};
#else
const std::vector<const char*> validationLayers = {
};
#endif // DEBUG

namespace glfw_inputs {
void onKeyboard(GLFWwindow * window, int key, int scancode, int action, int mods);
void onMouseMotion(GLFWwindow * window, double xpos, double ypos);
void onMouseButton(GLFWwindow * window, int button, int action, int mods);
}

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

Window::Window(unsigned int width, unsigned int height) {

    this->oldMouseX = 0;
    this->oldMouseY = 0;

    state.glfwWindow = vkutil::createWindow(width, height, this);

    glfwSetKeyCallback(state.glfwWindow, glfw_inputs::onKeyboard);
    glfwSetMouseButtonCallback(state.glfwWindow, glfw_inputs::onMouseButton);
    glfwSetCursorPosCallback(state.glfwWindow, glfw_inputs::onMouseMotion);

    //std::vector<const char *> validationLayers(0);
    state.instance = vkutil::createInstance(validationLayers);
    vkutil::setupDebugMessenger(state.instance, validationLayers.size());

    state.surface = vkutil::createSurface(state.instance, state.glfwWindow);
    state.physicalDevice = vkutil::pickPhysicalDevice(state.instance, [&] (VkPhysicalDevice & d) -> bool {
        return isDeviceSuitable(d, state.surface);
    });

    state.device = vkutil::createLogicalDevice(state.physicalDevice, state.surface, &state.graphicsQueue, &state.presentQueue, &state.transferQueue, &state.loadingGraphicsQueue, deviceExtensions);

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

void Window::onKeyboard(int key, int scancode, int action, int mods) {

    for (std::shared_ptr<InputHandler> h : inputHandlers) {

        h->onKeyboard(key, scancode, action, mods);

    }

}

void Window::onMouseButton(int button, int action, int mods) {

    for (std::shared_ptr<InputHandler> h : inputHandlers) {

        h->onMouseButton(button, action, mods);

    }

}

void Window::onMouseMotion(double xpos, double ypos) {

    double dx = xpos - oldMouseX;
    double dy = ypos - oldMouseY;

    oldMouseX = xpos;
    oldMouseY = ypos;

    for (std::shared_ptr<InputHandler> h : inputHandlers) {

        h->onMouseMotion(xpos, ypos, dx, dy);

    }

}

void Window::addInputHandler(std::shared_ptr<InputHandler> handler) {

    this->inputHandlers.push_back(handler);

}

void glfw_inputs::onKeyboard(GLFWwindow * w, int key, int scancode, int action, int mods) {

    Window * window = (Window *) glfwGetWindowUserPointer(w);
    window->onKeyboard(key, scancode, action, mods);

}

void glfw_inputs::onMouseButton(GLFWwindow * w, int button, int action, int mods) {

    Window * window = (Window *) glfwGetWindowUserPointer(w);
    window->onMouseButton(button, action, mods);

}

void glfw_inputs::onMouseMotion(GLFWwindow * w, double xpos, double ypos) {

    Window * window = (Window *) glfwGetWindowUserPointer(w);
    window->onMouseMotion(xpos, ypos);

}
