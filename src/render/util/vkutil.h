#ifndef VKUTIL_H
#define VKUTIL_H

#include <vector>
#include <functional>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>

namespace vkutil {

struct QueueFamilyIndices {

    int graphicsFamily = -1;
    int presentFamily = -1;

    bool isComplete() {
        return graphicsFamily >= 0 && presentFamily >= 0;
    }

};

struct SwapChainSupportDetails {

    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;

};

struct SwapChain {

    VkSwapchainKHR chain;
    std::vector<VkImage> images;
    VkFormat format;
    VkExtent2D extent;

};

GLFWwindow * createWindow(unsigned int width, unsigned int height, void * userData);
void destroyWindow(GLFWwindow * window);

VkInstance createInstance(std::vector<const char *> validationLayers);
VkSurfaceKHR createSurface(VkInstance instance, GLFWwindow * window);

QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice & device, const VkSurfaceKHR & surface);
bool checkDeviceExtensionSupport(VkPhysicalDevice pdevice, const std::vector<const char*> deviceExtensions);
SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice & device, const VkSurfaceKHR & surface);

VkPhysicalDevice pickPhysicalDevice(VkInstance instance, std::function<bool(VkPhysicalDevice &)> isDeviceSuitable);

VkDevice createLogicalDevice(VkPhysicalDevice & pDevice, VkSurfaceKHR & surface, VkQueue * gQueue, VkQueue * pQueue, const std::vector<const char*> deviceExtensions);

VmaAllocator createAllocator(VkDevice & device, VkPhysicalDevice & pDevice);

SwapChain createSwapchain(const VkPhysicalDevice & physicalDevice, const VkDevice & device, const VkSurfaceKHR & surface, GLFWwindow * window);

VkCommandPool createCommandPool(const VkPhysicalDevice & physicalDevice, const VkDevice & device, const VkSurfaceKHR & surface);

void createImage(const VmaAllocator & allocator, VkDevice device, int width, int height, int depth, int mipLevels, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlagBits memProps, VkImage & image, VmaAllocation & memory);
VkImageView createImageView(const VkDevice & device, const VkImage & image, VkFormat format, VkImageAspectFlags aspect, int mipLevels);
void transitionImageLayout(const VkImage & image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, int mipLevels, const VkCommandPool & commandPool, const VkDevice & device, const VkQueue & q);

VkCommandBuffer beginSingleCommand(const VkCommandPool & commandPool, const VkDevice & device);
void endSingleCommand(VkCommandBuffer & commandBuffer, const VkCommandPool & commandPool, const VkDevice & device, const VkQueue & q);

}

#endif // VKUTIL_H
