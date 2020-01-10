#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <memory>

#include "util/vkutil.h"
#include "window.h"

class Viewport
{
    public:
        Viewport(std::shared_ptr<Window> window);
        virtual ~Viewport();

    protected:

        void setupRenderPass();

    private:

        vkutil::SwapChain swapchain;

        //const VkInstance & instance;
        //const VkSurfaceKHR & surface;
        //const VkPhysicalDevice & physicalDevice;
        //const VkQueue & presentQueue;
        const VkQueue & graphicsQueue;
        const VkDevice & device;
        const VmaAllocator & vmaAllocator;
        const VkCommandPool & commandPool;
        VkRenderPass renderPass;

        VkImage depthImage;
        VkImageView depthImageView;
        VmaAllocation depthImageMemory;

};

#endif // VIEWPORT_H
