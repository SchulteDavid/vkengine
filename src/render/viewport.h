#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <memory>

#include <glm/glm.hpp>

#include "render/util/vkutil.h"
#include "window.h"
#include "model.h"
#include "renderelement.h"
#include "camera.h"

class Viewport : public MemoryTransferHandler
{
    public:
        Viewport(std::shared_ptr<Window> window, Camera * camera);
        virtual ~Viewport();

        struct LightData {

            alignas(16) glm::vec4 position[32];
            alignas(16) glm::vec4 color[32];

        };

        struct CameraData;

        void drawFrame(bool updateElements = true);

        vkutil::VulkanState & getState();
        const VkRenderPass & getRenderpass();
        const VkExtent2D & getSwapchainExtent();
        unsigned int getSwapchainSize();

        void addRenderElement(std::shared_ptr<RenderElement> rElem);

        void addLight(glm::vec4 pos, glm::vec4 color);

        void manageMemoryTransfer();

        Camera * getCamera();

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

            std::vector<VkFramebuffer> framebuffers;
            std::vector<VkImageView> imageViews;


        } swapchain;

        vkutil::VulkanState & state;

        Camera * camera;
        bool framebufferResized;

        Window * window;
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

        std::shared_ptr<Model> ppBufferModel;

        LightData lights;
        unsigned int lightIndex;

        std::vector<std::shared_ptr<RenderElement>> renderElements;
        std::unordered_map<Shader *, std::vector<std::shared_ptr<RenderElement>>> renderElementsByShader;

};

#endif // VIEWPORT_H
