#ifndef TEXTURE_H
#define TEXTURE_H

#include <vector>
#include <string>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "render/util/vkutil.h"

#include "resources/resourceuploader.h"
#include "resources/resourceloader.h"

class Texture : public Resource
{
    public:
        Texture(vkutil::VulkanState & state, const std::vector<float> & data, int width, int height, int depth);
        Texture(vkutil::VulkanState & state, const std::vector<uint8_t> & data, int width, int height, int depth);
        virtual ~Texture();

        void transitionLayout(vkutil::VulkanState & state, VkImageLayout layout);

        VkImageView & getView();
        VkImageLayout getLayout();

        VkSampler & getSampler();

        static Texture * createTexture(vkutil::VulkanState & state, std::string fname);

        static void createImage(vkutil::VulkanState & state, int width, int height, int depth, int mipLevels, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlagBits memProps, VkImage & image, VmaAllocation & memory);
        static void copyBufferToImage(const vkutil::VulkanState & state, VkBuffer & buffer, VkImage & image, uint32_t width, uint32_t height, uint32_t depth);
        static VkImageView createImageView(const vkutil::VulkanState & state, VkImage & image, VkFormat format, VkImageAspectFlags aspect, int mipLevels);
        static VkSampler createSampler(const vkutil::VulkanState & state, int mipLevels);
        static void transitionImageLayout(vkutil::VulkanState & state, VkImage & image, VkFormat format, VkImageLayout layout, VkImageLayout newLayout, int mipLevels);

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

template <typename T> class TextureUploader : public ResourceUploader<Texture> {

    public:
        TextureUploader(vkutil::VulkanState & state, std::vector<T> data, int width, int height, int depth) : state(state) {

            this->data = data;
            this->width = width;
            this->height = height;
            this->depth = depth;

        }

        bool uploadReady() {
            return true;
        }

        std::shared_ptr<Texture> uploadResource() {
            return std::shared_ptr<Texture>(new Texture(state, data, width, height, depth));
        }

    private:

        vkutil::VulkanState & state;
        std::vector<T> data;
        int width;
        int height;
        int depth;

};

class TextureLoader : public ResourceLoader<Texture> {

    public:
        TextureLoader(vkutil::VulkanState & state);

        std::shared_ptr<ResourceUploader<Texture>> loadResource(std::string fname);

    protected:
        vkutil::VulkanState & state;

};

class PNGLoader : public TextureLoader {

    public:
        PNGLoader(vkutil::VulkanState & state);
        std::shared_ptr<ResourceUploader<Texture>> loadResource(std::string fname);

};

#endif // TEXTURE_H
