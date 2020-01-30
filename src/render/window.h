#ifndef WINDOW_H
#define WINDOW_H

#include <memory>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "render/util/vkutil.h"
#include "inputs/inputhandler.h"

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

        void addInputHandler(std::shared_ptr<InputHandler> handler);

        void onKeyboard(int key, int scancode, int action, int mods);
        void onMouseButton(int button, int action, int mods);
        void onMouseMotion(double xpos, double ypos);

    protected:

    private:

        vkutil::VulkanState state;

        std::vector<std::shared_ptr<InputHandler>> inputHandlers;
        double oldMouseX;
        double oldMouseY;


};

#endif // WINDOW_H
