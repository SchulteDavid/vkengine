#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <memory>

#include <glm/glm.hpp>

#include "util/vkutil.h"
#include "window.h"

class Viewport
{
    public:
        Viewport(std::shared_ptr<Window> window);
        virtual ~Viewport();

        struct LightData {

            alignas(16) glm::vec4 position[32];
            alignas(16) glm::vec4 color[32];

        };

        struct CameraData;

        void drawFrame();

    protected:

        void setupRenderPass();
        void createPPObjects();
        void destroyPPObjects();

        void createPpDescriptorSetLayout();
        void createPpDescriptorPool();
        void createPPDescriptorSets();

        void setupPostProcessingPipeline();
        void setupFramebuffers();

        void destroySwapChain();
        void recreateSwapChain();

        void recordCommandBuffers();
        void setupCommandBuffers();
        void createSyncObjects();

        void createTransferCommandBuffer();

        void prepareRenderElements();

        void updateUniformBuffer(uint32_t frameIndex);

    private:

        struct SwapchainInfo : vkutil::SwapChain {

            /*VkSwapchainKHR chain;
            std::vector<VkImage> images;
            VkFormat format;
            VkExtent2D extent;*/


            std::vector<VkFramebuffer> framebuffers;
            std::vector<VkImageView> imageViews;


        } swapchain;

        void * camera;
        bool framebufferResized;

        Window * window;

        //const VkInstance & instance;
        //const VkSurfaceKHR & surface;
        //const VkPhysicalDevice & physicalDevice;
        const VkQueue & presentQueue;
        const VkQueue & graphicsQueue;
        const VkDevice & device;
        const VmaAllocator & vmaAllocator;
        const VkCommandPool & commandPool;
        VkRenderPass renderPass;

        VkImage depthImage;
        VkImageView depthImageView;
        VmaAllocation depthImageMemory;

        VkImage gBufferImage;
        VkImageView gBufferImageView;
        VmaAllocation gBufferImageMemory;

        VkImage nBufferImage;
        VkImageView nBufferImageView;
        VmaAllocation nBufferImageMemory;

        VkImage aBufferImage;
        VkImageView aBufferImageView;
        VmaAllocation aBufferImageMemory;

        std::vector<VkBuffer> ppLightBuffers;
        std::vector<VmaAllocation> ppLightBuffersMemory;
        std::vector<VkBuffer> ppCameraBuffers;
        std::vector<VmaAllocation> ppCameraBuffersMemory;

        VkDescriptorSetLayout ppDescLayout;
        VkDescriptorPool ppDescPool;
        std::vector<VkDescriptorSet> ppDescSets;

        VkPipelineLayout ppPipelineLayout;
        VkPipeline ppPipeline;

        std::vector<VkCommandBuffer> commandBuffers;
        VkCommandBuffer transferCmdBuffer;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        unsigned int frameIndex;
        std::vector<VkFence> inFlightFences;
        VkFence transferFence;

};

#endif // VIEWPORT_H
