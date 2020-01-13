#ifndef WINDOW_H
#define WINDOW_H

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "util/vkutil.h"

class Window
{
    public:
        Window();
        virtual ~Window();

        VkPhysicalDevice & getPhysicalDevice();
        VkInstance & getInstance();
        VkSurfaceKHR & getSurface();
        VkQueue & getGraphicsQueue();
        VkQueue & getPresentQueue();
        VkDevice & getDevice();
        VmaAllocator & getAllocator();
        VkCommandPool & getCommandPool();
        GLFWwindow * getGlfwWindow();

        vkutil::VulkanState & getState();

    protected:

    private:

        vkutil::VulkanState state;


};

#endif // WINDOW_H
