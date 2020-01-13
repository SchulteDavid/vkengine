#ifndef TEXTURE_H
#define TEXTURE_H

#include <vector>
#include <string>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "util/vkutil.h"

class Texture
{
    public:
        Texture(const vkutil::VulkanState & state, const std::vector<float> & data, int width, int height, int depth);
        virtual ~Texture();

        void transitionLayout(const vkutil::VulkanState & state, VkImageLayout layout);

        VkImageView & getView();
        VkImageLayout getLayout();

        VkSampler & getSampler();

        static Texture * createTexture(const vkutil::VulkanState & state, std::string fname);

        static void createImage(const vkutil::VulkanState & state, int width, int height, int depth, int mipLevels, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlagBits memProps, VkImage & image, VmaAllocation & memory);
        static void copyBufferToImage(const vkutil::VulkanState & state, VkBuffer & buffer, VkImage & image, uint32_t width, uint32_t height, uint32_t depth);
        static VkImageView createImageView(const vkutil::VulkanState & state, VkImage & image, VkFormat format, VkImageAspectFlags aspect, int mipLevels);
        static VkSampler createSampler(const vkutil::VulkanState & state, int mipLevels);
        static void transitionImageLayout(const vkutil::VulkanState & state, VkImage & image, VkFormat format, VkImageLayout layout, VkImageLayout newLayout, int mipLevels);

    protected:

        VkImage image;
        VmaAllocation memory;
        VkImageLayout layout;
        VkFormat format;
        VkImageView view;

        int mipLevels;
        VkSampler sampler;

    private:

        const VkDevice & device;
        const VmaAllocator & allocator;

        void generateMipmaps(int width, int height, const VkCommandPool & commandPool, const VkDevice & device, const VkQueue & q);

};

#endif // TEXTURE_H
