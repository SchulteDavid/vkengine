#ifndef WINDOW_H
#define WINDOW_H

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

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

    protected:

    private:

        GLFWwindow * glfwWindow;
        VkInstance instance;
        VkSurfaceKHR surface;
        VkPhysicalDevice physicalDevice;
        VkQueue presentQueue;
        VkQueue graphicsQueue;
        VkDevice device;
        VmaAllocator vmaAllocator;
        VkCommandPool commandPool;


};

#endif // WINDOW_H
