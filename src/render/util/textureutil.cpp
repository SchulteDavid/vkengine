#include "render/util/vkutil.h"

#include "vk_trace_exception.h"

#include "util/debug/trace_exception.h"
#include <string.h>
#include <algorithm>
#include <cmath>

using namespace vkutil;

namespace vkutil {

  struct CubemapCreateData {
    VkImage image;
    VkFormat format;
    VkImageView imageView;
    VkSampler sampler;
  };
  
  CubemapCreateData createCubemap(VulkanState & state, std::vector<uint8_t> rawData, uint32_t width, uint32_t height, uint32_t channelCount);
};

CubemapCreateData vkutil::createCubemap(VulkanState & state, std::vector<uint8_t> rawData, uint32_t width, uint32_t height, uint32_t channelCount) {

  /// Check if data can match the given width and height
  if (rawData.size() != (width * height * channelCount * 6))
    throw dbg::trace_exception("Cubemap data does not match image size and channel count.");


  /// Precompute some usefull values
  VkDeviceSize imageSize = sizeof(uint8_t) * rawData.size();
  //uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
  uint32_t mipLevels = 1;
  VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;


  /// Create and fill a staging buffer for more efficient transfer.
  VkBuffer stagingBuffer;
  VmaAllocation stagingBufferMemory;
  
  VkBufferCreateInfo stBufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  stBufferCreateInfo.size = imageSize;
  stBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  stBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  
  VmaAllocationCreateInfo stAllocCreateInfo = {};
  stAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
  stAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
  
  VmaAllocationInfo stagingBufferAllocInfo = {};
  
  vmaCreateBuffer(state.vmaAllocator, &stBufferCreateInfo, &stAllocCreateInfo, &stagingBuffer, &stagingBufferMemory, &stagingBufferAllocInfo);
  memcpy(stagingBufferAllocInfo.pMappedData, rawData.data(), imageSize);


  /// Create the image
  VkImageCreateInfo imageInfo = {};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = mipLevels;
  imageInfo.arrayLayers = 6; // cubes have 6 faces
  imageInfo.format = format;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT; //
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; // mark as cubemap
  
  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  
  VkResult retVal;

  VkImage image;
  VmaAllocation memory;
  
  if (retVal = vmaCreateImage(state.vmaAllocator, &imageInfo, &allocInfo, &image, &memory, nullptr))
    throw vkutil::vk_trace_exception("Could not create image", retVal);

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(state.device, image, &memRequirements);


  /// Copy image from buffer to image memory
  VkCommandBuffer commandBuffer = vkutil::beginSingleCommand(state.transferCommandPool, state.device);
  
  std::vector<VkBufferImageCopy> regions(6);
  for (unsigned int i = 0; i < 6; ++i) {
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferImageHeight = 0;
    region.bufferRowLength = 0;
  
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageExtent = {width, height, 1};
    region.imageOffset = {0, 0, 0};
    regions[i] = region;
  }

  vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.size(), regions.data());
  
  vkutil::endSingleCommand(commandBuffer, state.transferCommandPool, state.device, state.transferQueue);


  /// Create a sampler for this image
  VkSamplerCreateInfo samplerInfo = {};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;

  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = 16;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0;
  samplerInfo.maxLod = mipLevels;

  VkSampler sampler;

  if (vkCreateSampler(state.device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
    throw dbg::trace_exception("Unable to create sampler");


  /// Create image view
  VkImageViewCreateInfo viewInfo = {};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = format;
  
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = mipLevels;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 6;
  
  VkImageView imageView;
  if (VkResult r = vkCreateImageView(state.device, &viewInfo, nullptr, &imageView))
    throw vkutil::vk_trace_exception("Unable to create image view", r);
  

  /// Cleanup the mess
  vmaDestroyBuffer(state.vmaAllocator, stagingBuffer, stagingBufferMemory);

  CubemapCreateData retData;
  retData.image = image;
  retData.format = format;
  retData.sampler = sampler;
  retData.imageView = imageView;

  return retData;
  
}
