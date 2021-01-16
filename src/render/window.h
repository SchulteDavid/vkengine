#ifndef WINDOW_H
#define WINDOW_H

#include <memory>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "render/util/vkutil.h"
#include "inputs/inputhandler.h"

class Viewport;
class Camera;

class Window
{
    public:
        Window(unsigned int width, unsigned int height);
        virtual ~Window();

        VkPhysicalDevice & getPhysicalDevice();
        VkInstance & getInstance();
        VkSurfaceKHR & getSurface();
        vkutil::Queue & getGraphicsQueue();
        vkutil::Queue & getPresentQueue();
        VkDevice & getDevice();
        VmaAllocator & getAllocator();
        VkCommandPool & getCommandPool();
        GLFWwindow * getGlfwWindow();

        vkutil::VulkanState & getState();

        void addInputHandler(std::shared_ptr<InputHandler> handler);

        void onKeyboard(int key, int scancode, int action, int mods);
        void onMouseButton(int button, int action, int mods);
        void onMouseMotion(double xpos, double ypos);
        void onScroll(double dx, double dy);

        void hideCursor();
        void unhideCursor();

        void setActiveViewport(Viewport * view);
        Viewport * getActiveViewport();
        std::shared_ptr<Camera> getActiveCamera();

    protected:

    private:

        vkutil::VulkanState state;

        std::vector<std::shared_ptr<InputHandler>> inputHandlers;
        double oldMouseX;
        double oldMouseY;

        Viewport * view;


};

#endif // WINDOW_H
