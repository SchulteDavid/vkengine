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
    int transferFamily = -1;

    bool isComplete() {
        return graphicsFamily >= 0 && presentFamily >= 0 && transferFamily >= 0 && transferFamily != graphicsFamily;
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

struct VertexInputDescriptions {
    std::vector<VkVertexInputBindingDescription> binding;
    std::vector<VkVertexInputAttributeDescription> attributes;
};

struct ShaderInputDescription {
    VkShaderModule module;
    VkShaderStageFlagBits usage;
    char * entryName;
};

struct VulkanState {

    GLFWwindow * glfwWindow;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    VkQueue presentQueue;
    VkQueue graphicsQueue;
    VkQueue transferQueue;
    VkDevice device;
    VmaAllocator vmaAllocator;
    VkCommandPool graphicsCommandPool;
    VkCommandPool transferCommandPool;

};

GLFWwindow * createWindow(unsigned int width, unsigned int height, void * userData);
void destroyWindow(GLFWwindow * window);

void setupDebugMessenger(const VkInstance & instance, bool validationLayers);

VkInstance createInstance(std::vector<const char *> validationLayers);
VkSurfaceKHR createSurface(VkInstance instance, GLFWwindow * window);

QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice & device, const VkSurfaceKHR & surface);
bool checkDeviceExtensionSupport(VkPhysicalDevice pdevice, const std::vector<const char*> deviceExtensions);
SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice & device, const VkSurfaceKHR & surface);

VkPhysicalDevice pickPhysicalDevice(VkInstance instance, std::function<bool(VkPhysicalDevice &)> isDeviceSuitable);

VkDevice createLogicalDevice(VkPhysicalDevice & pDevice, VkSurfaceKHR & surface, VkQueue * gQueue, VkQueue * pQueue, VkQueue * tQueue, const std::vector<const char*> deviceExtensions);

VmaAllocator createAllocator(VkDevice & device, VkPhysicalDevice & pDevice);

SwapChain createSwapchain(const VulkanState & state);
SwapChain createSwapchain(const VkPhysicalDevice & physicalDevice, const VkDevice & device, const VkSurfaceKHR & surface, GLFWwindow * window);
std::vector<VkImageView> createSwapchainImageViews(const std::vector<VkImage> & swapChainImages, VkFormat swapChainFormat, const VkDevice & device);

VkCommandPool createGraphicsCommandPool(const VkPhysicalDevice & physicalDevice, const VkDevice & device, const VkSurfaceKHR & surface);
VkCommandPool createTransferCommandPool(const VkPhysicalDevice & physicalDevice, const VkDevice & device, const VkSurfaceKHR & surface);

void createImage(const VmaAllocator & allocator, VkDevice device, int width, int height, int depth, int mipLevels, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlagBits memProps, VkImage & image, VmaAllocation & memory);
VkImageView createImageView(const VkDevice & device, const VkImage & image, VkFormat format, VkImageAspectFlags aspect, int mipLevels);
void transitionImageLayout(const VkImage & image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, int mipLevels, const VkCommandPool & commandPool, const VkDevice & device, const VkQueue & q);

VkCommandBuffer beginSingleCommand(const VulkanState & state);
void endSingleCommand(VkCommandBuffer & commandBuffer,const VulkanState & state);

VkCommandBuffer beginSingleCommand(const VkCommandPool & commandPool, const VkDevice & device);
void endSingleCommand(VkCommandBuffer & commandBuffer, const VkCommandPool & commandPool, const VkDevice & device, const VkQueue & q);

VkShaderModule createShaderModule(const std::vector<uint8_t> & code, const VkDevice & device);

void copyBuffer(VkBuffer & src, VkBuffer & dst, VkDeviceSize & size, const VkCommandPool & commandPool, const VkDevice & device, const VkQueue & q);
VkDescriptorSetLayout createDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding> & bindings, const VkDevice & device);
VkPipeline createGraphicsPipeline(const VulkanState & state, const VkRenderPass & renderPass, std::vector<ShaderInputDescription>& shaders, VertexInputDescriptions & descs, VkDescriptorSetLayout & descriptorSetLayout, VkPipelineLayout & retLayout, VkExtent2D swapChainExtent);

bool hasStencilComponent(VkFormat format);

}

std::vector<uint8_t> readFile(const std::string & fname);

#endif // VKUTIL_H
