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
  static void copyBufferToImage(vkutil::VulkanState & state, VkBuffer & buffer, VkImage & image, uint32_t width, uint32_t height, uint32_t depth, uint32_t layerCount);
  static VkImageView createImageView(const vkutil::VulkanState & state, VkImage & image, VkFormat format, VkImageAspectFlags aspect, int mipLevels);
  static VkSampler createSampler(const vkutil::VulkanState & state, int mipLevels);
  static void transitionImageLayout(vkutil::VulkanState & state, VkImage & image, VkFormat format, VkImageLayout layout, VkImageLayout newLayout, int mipLevels);

protected:

  Texture(vkutil::VulkanState & state, const std::vector<uint8_t> & data, int width, int height, int depth, VkImageViewType type, int layerCount, VkImageCreateFlags flags);

  VkImage image;
  VmaAllocation memory;
  VkImageLayout layout;
  VkFormat format;
  VkImageView view;

  int mipLevels;
  VkSampler sampler;
  int layerCount;

private:

  const VkDevice & device;
  const VmaAllocator & allocator;

  void generateMipmaps(int width, int height, const VkCommandPool & commandPool, const VkDevice & device, const vkutil::Queue & q);

};

template <typename T> class TextureUploader : public ResourceUploader<Texture> {

public:
  TextureUploader(std::vector<T> data, int width, int height, int depth) {

    this->data = data;
    this->width = width;
    this->height = height;
    this->depth = depth;

  }

  bool uploadReady() {
    return true;
  }

  std::shared_ptr<Texture> uploadResource(vkutil::VulkanState & state, ResourceManager * manager) {
    return std::shared_ptr<Texture>(new Texture(state, data, width, height, depth));
  }

private:

  std::vector<T> data;
  int width;
  int height;
  int depth;

};

class TextureLoader : public ResourceLoader<Texture> {

public:
  TextureLoader();

  std::shared_ptr<ResourceUploader<Texture>> loadResource(std::string fname);


};

class PNGLoader : public TextureLoader {

public:
  PNGLoader();
  std::shared_ptr<ResourceUploader<Texture>> loadResource(std::string fname);

};

#endif // TEXTURE_H
