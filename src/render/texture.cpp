#include "texture.h"
#include <stdlib.h>
#include <string.h>
#include <limits>
#include "util/vkutil.h"
#include "storagebuffer.h"

#include <tga.h>
#include <cmath>

#include <iostream>

#include "util/debug/trace_exception.h"

using namespace vkutil;

Texture::Texture(vkutil::VulkanState & state, const std::vector<float> & data, int width, int height, int depth) : allocator(state.vmaAllocator), device(state.device) {

    format = VK_FORMAT_R32G32B32A32_SFLOAT;
    layout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkDeviceSize imageSize = sizeof(float) * data.size();
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

    vmaCreateBuffer(allocator, &stBufferCreateInfo, &stAllocCreateInfo, &stagingBuffer, &stagingBufferMemory, &stagingBufferAllocInfo);

    memcpy(stagingBufferAllocInfo.pMappedData, data.data(), imageSize);

    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    vkutil::createImage(allocator, device, width, height, depth, mipLevels, format, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory);

    this->transitionLayout(state, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(state, stagingBuffer, image, width, height, depth);
    //this->transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    std::cout << "Generating mipmaps" << std::endl;
    state.graphicsQueueMutex.lock();
    generateMipmaps(width, height, state.graphicsCommandPool, device, state.graphicsQueue);
    state.graphicsQueueMutex.unlock();
    std::cout << "done" << std::endl;

    this->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    //this->transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferMemory);
    /*vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);*/

    view = vkutil::createImageView(device, image, format, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

    this->sampler = createSampler(state, mipLevels);

}

Texture::Texture(vkutil::VulkanState & state, const std::vector<uint8_t> & data, int width, int height, int depth) : allocator(state.vmaAllocator), device(state.device) {

    //format = VK_FORMAT_R32G32B32A32_SFLOAT;
    format = VK_FORMAT_R8G8B8A8_UNORM;
    layout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkDeviceSize imageSize = sizeof(uint8_t) * data.size();
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

    vmaCreateBuffer(allocator, &stBufferCreateInfo, &stAllocCreateInfo, &stagingBuffer, &stagingBufferMemory, &stagingBufferAllocInfo);

    memcpy(stagingBufferAllocInfo.pMappedData, data.data(), imageSize);

    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;


    vkutil::createImage(allocator, device, width, height, depth, mipLevels, format, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory);

    state.graphicsQueueMutex.lock();
    this->transitionLayout(state, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    state.graphicsQueueMutex.unlock();
    copyBufferToImage(state, stagingBuffer, image, width, height, depth);
    //this->transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    std::cout << "Generating mipmaps" << std::endl;
    state.graphicsQueueMutex.lock();
    generateMipmaps(width, height, state.graphicsCommandPool, device, state.graphicsQueue);
    state.graphicsQueueMutex.unlock();
    std::cout << "done" << std::endl;

    this->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    //this->transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferMemory);
    /*vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);*/

    view = vkutil::createImageView(device, image, format, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

    this->sampler = createSampler(state, mipLevels);

}

Texture::~Texture() {

    vkDestroyImageView(device, view, nullptr);
    /*vkDestroyImage(device, image, nullptr);
    vkFreeMemory(device, memory, nullptr);*/
    vmaDestroyImage(allocator, image, memory);

}

void Texture::generateMipmaps(int width, int height, const VkCommandPool & commandPool, const VkDevice & device, const VkQueue & q) {

    VkCommandBuffer cmdBuffer = vkutil::beginSingleCommand(commandPool, device);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = this->image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    uint32_t mipWidth = width;
    uint32_t mipHeight = height;

    for (unsigned int i = 1; i < mipLevels; ++i) {

        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);


        VkImageBlit blit = {};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = {(int32_t) mipWidth, (int32_t) mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { (int32_t) (mipWidth > 1 ? mipWidth / 2 : 1), (int32_t) (mipHeight > 1 ? mipHeight / 2 : 1), 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(cmdBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        if (mipHeight > 1) mipHeight /= 2;
        if (mipWidth > 1) mipWidth /= 2;


    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkutil::endSingleCommand(cmdBuffer, commandPool, device, q);

}

VkSampler & Texture::getSampler() {
    return sampler;
}

void Texture::copyBufferToImage(const VulkanState & state, VkBuffer & buffer, VkImage & image, uint32_t width, uint32_t height, uint32_t depth) {

    VkCommandBuffer commandBuffer = vkutil::beginSingleCommand(state.transferCommandPool, state.device);

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferImageHeight = 0;
    region.bufferRowLength = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageExtent = {width, height, depth};
    region.imageOffset = {0,0,0};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    vkutil::endSingleCommand(commandBuffer, state.transferCommandPool, state.device, state.transferQueue);

}

void Texture::transitionLayout(const VulkanState & state, VkImageLayout newLayout) {


    transitionImageLayout(state, image, format, layout, newLayout, mipLevels);

    layout = newLayout;

}

VkImageView & Texture::getView() {
    return view;
}

VkImageLayout Texture::getLayout() {
    return layout;
}

/** static helper functions **/

void Texture::createImage(VulkanState & state, int width, int height, int depth, int mipLevels, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlagBits memProps, VkImage & image, VmaAllocation & memory) {

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = depth;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage; //
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.requiredFlags = memProps;

    int retVal;

    if ((retVal = vmaCreateImage(state.vmaAllocator, &imageInfo, &allocInfo, &image, &memory, nullptr)) != VK_SUCCESS)
        throw dbg::trace_exception(std::string("Could not create image ").append(std::to_string(retVal)));

    /*if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
        throw dbg::trace_exception("Unable to create texture");

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = StorageBuffer::findMemoryType(memRequirements.memoryTypeBits, memProps); //VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT

    if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
        throw dbg::trace_exception("Unable to allocate memory for image");

    vkBindImageMemory(device, image, memory, 0);*/

}

VkImageView Texture::createImageView(const VulkanState & state, VkImage & image, VkFormat format, VkImageAspectFlags aspect, int mipLevels) {

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;

    viewInfo.subresourceRange.aspectMask = aspect;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(state.device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
        throw dbg::trace_exception("Unable to create image view");

    return imageView;

}

VkSampler Texture::createSampler(const VulkanState & state, int mipLevels) {

    std::cout << "Creating sampler with " << mipLevels << " mip levels" << std::endl;

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

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

    return sampler;

}

void Texture::transitionImageLayout(const VulkanState & state, VkImage & image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, int mipLevels) {

    VkCommandBuffer commandBuffer = vkutil::beginSingleCommand(state);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    vkutil::endSingleCommand(commandBuffer, state);

}

std::vector<float> convertTgaDataToFloat(uint8_t * data, int width, int height) {

    std::vector<float> dest(width * height * 4);

    for (int i = 0; i < (width * height * 4); ++i)
        dest[i] = (float) data[i] / (float) std::numeric_limits<uint8_t>::max();

    return dest;

}

Texture * Texture::createTexture(vkutil::VulkanState & state, std::string fname) {

    TGA_FILE * tgaImage = tgaOpen(fname.c_str());
    int width, height;
    tgaGetSize(tgaImage, &width, &height);
    uint8_t * rawData = tgaGetColorDataRGBA(tgaImage);
    std::vector<float> imageData = convertTgaDataToFloat(rawData, width, height);
    free(rawData);
    tgaClose(tgaImage);

    return new Texture(state, imageData, width, height, 1);

}

TextureLoader::TextureLoader(vkutil::VulkanState & state) : state(state) {



}

std::shared_ptr<ResourceUploader<Texture>> TextureLoader::loadResource(std::string fname) {

    if (fname.substr(fname.length() - 3).compare("tga")) {
        throw std::exception();
    }

    TGA_FILE * tgaImage = tgaOpen(fname.c_str());
    int width, height;
    tgaGetSize(tgaImage, &width, &height);
    uint8_t * rawData = tgaGetColorDataRGBA(tgaImage);
    std::vector<float> imageData = convertTgaDataToFloat(rawData, width, height);
    free(rawData);
    tgaClose(tgaImage);

    return std::shared_ptr<ResourceUploader<Texture>>(new TextureUploader<float>(state, imageData, width, height, 1));

}

PNGLoader::PNGLoader(vkutil::VulkanState & state) : TextureLoader(state) {

}

#include "util/image/png.h"

std::shared_ptr<ResourceUploader<Texture>> PNGLoader::loadResource(std::string fname) {

    FILE * file = fopen(fname.c_str(), "rb");

    uint32_t width, height, chanelCount;
    uint8_t * data = pngLoadImageData(file, &width, &height, &chanelCount);

    if (!data)
        throw dbg::trace_exception("Unable to load PNG");

    fclose(file);

    /*std::vector<float> fData(width * height * 4);

    for (unsigned int i = 0; i < height; ++i) {
        for (unsigned int j = 0; j < width; ++j) {
            for (unsigned int c = 0; c < chanelCount; ++c) {
                fData[(i * width + j) * 4 + c] = (float) data[(i * width + j) * chanelCount + c] / 255.0;
            }

            if (chanelCount < 4) {
                fData[(i * width + j) * 4 + 3] = 1.0;
            }

        }
        //fData[i] = (float) data[i] / 255.0;
    }*/

    std::vector<uint8_t> fData(width * height * 4);

    for (unsigned int i = 0; i < height; ++i) {
        for (unsigned int j = 0; j < width; ++j) {
            for (unsigned int c = 0; c < chanelCount; ++c) {
                fData[(i * width + j) * 4 + c] = data[(i * width + j) * chanelCount + c];
            }

            if (chanelCount < 4) {
                fData[(i * width + j) * 4 + 3] = 255;
            }

        }
        //fData[i] = (float) data[i] / 255.0;
    }

    free(data);

    return std::shared_ptr<ResourceUploader<Texture>>(new TextureUploader<uint8_t>(state, fData, width, height, 1));

}
