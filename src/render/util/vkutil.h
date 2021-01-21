#ifndef VKUTIL_H
#define VKUTIL_H

#include <vector>
#include <functional>
#include <iostream>
#include <mutex>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>

#include "util/debug/logger.h"

class Window;

namespace vkutil {

  struct QueueFamilyIndices {

    int graphicsFamily = -1;
    int presentFamily = -1;
    int transferFamily = -1;

    std::vector<int> counts;

    bool isComplete() {
      lerr << (transferFamily == graphicsFamily ? "transfer is graphics" : "transfer != graphics") << std::endl;
      return graphicsFamily >= 0 && presentFamily >= 0 && transferFamily >= 0;// && transferFamily != graphicsFamily;
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

  class Queue {

  public:
    Queue(std::mutex & m) : m(&m) {

    }

    VkQueue q;

    void lock() const;
    void unlock() const;

    void setMutex(std::mutex & m);


  private:
    std::mutex * m;

  };
  
  struct VulkanState {
    
    VulkanState() :
      presentQueue(presentQueueMutex),
      graphicsQueue(graphicsQueueMutex),
      loadingGraphicsQueue(loadingGraphicsQueueMutex),
      transferQueue(graphicsQueueMutex)
    {
      
    }
    
    GLFWwindow * glfwWindow;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    Queue presentQueue;
    Queue graphicsQueue;
    Queue loadingGraphicsQueue;
    Queue transferQueue;
    VkDevice device;
    VmaAllocator vmaAllocator;
    VkCommandPool graphicsCommandPool;
    VkCommandPool loadingCommandPool;
    VkCommandPool transferCommandPool;
    Window * window;

    std::mutex graphicsQueueMutex;
    std::mutex transferQueueMutex;
    std::mutex loadingGraphicsQueueMutex;
    std::mutex presentQueueMutex;
    
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
  
  VkDevice createLogicalDevice(VkPhysicalDevice & pDevice, VkSurfaceKHR & surface, VkQueue * gQueue, VkQueue * pQueue, VkQueue * tQueue, VkQueue * lgQueue, const std::vector<const char*> deviceExtensions);
  
  VmaAllocator createAllocator(VkDevice & device, VkPhysicalDevice & pDevice);
  
  SwapChain createSwapchain(const VulkanState & state);
  SwapChain createSwapchain(const VkPhysicalDevice & physicalDevice, const VkDevice & device, const VkSurfaceKHR & surface, GLFWwindow * window);
  std::vector<VkImageView> createSwapchainImageViews(const std::vector<VkImage> & swapChainImages, VkFormat swapChainFormat, const VkDevice & device);
  
  VkCommandPool createGraphicsCommandPool(const VkPhysicalDevice & physicalDevice, const VkDevice & device, const VkSurfaceKHR & surface);
  VkCommandPool createSecondaryCommandPool(const VkPhysicalDevice & physicalDevice, const VkDevice & device, const VkSurfaceKHR & surface);
  VkCommandPool createTransferCommandPool(const VkPhysicalDevice & physicalDevice, const VkDevice & device, const VkSurfaceKHR & surface);

  void createImage(const VmaAllocator & allocator, VkDevice device, int width, int height, int depth, int mipLevels, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlagBits memProps, VkImage & image, VmaAllocation & memory, VkImageCreateFlags flags = 0, int layerCount = 1);
  VkImageView createImageView(const VkDevice & device, const VkImage & image, VkFormat format, VkImageAspectFlags aspect, int mipLevels, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, int layerCount = 1);
  void transitionImageLayout(const VkImage & image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, int mipLevels, const VkCommandPool & commandPool, const VkDevice & device, const Queue & q, int layerCount = 1);

  VkCommandBuffer beginSingleCommand(const VulkanState & state);
  void endSingleCommand(VkCommandBuffer & commandBuffer, VulkanState & state);

  VkCommandBuffer beginSingleCommand(const VkCommandPool & commandPool, const VkDevice & device);
  void endSingleCommand(VkCommandBuffer & commandBuffer, const VkCommandPool & commandPool, const VkDevice & device, const vkutil::Queue & q);

  VkShaderModule createShaderModule(const std::vector<uint8_t> & code, const VkDevice & device);

  void copyBuffer(VkBuffer & src, VkBuffer & dst, VkDeviceSize & size, const VkCommandPool & commandPool, const VkDevice & device, const Queue & q);
  VkDescriptorSetLayout createDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding> & bindings, const VkDevice & device);
  VkPipeline createGraphicsPipeline(const VulkanState & state, const VkRenderPass & renderPass, const std::vector<ShaderInputDescription> & shaders, const VertexInputDescriptions & descs, const VkDescriptorSetLayout & descriptorSetLayout, VkPipelineLayout & retLayout, VkExtent2D swapChainExtent, uint32_t subpassId);


  VkPipeline createComputePipeline(const VulkanState & state);
  
  bool hasStencilComponent(VkFormat format);

}

std::vector<uint8_t> readFile(const std::string & fname);

#endif // VKUTIL_H
