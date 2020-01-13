#ifndef STORAGEBUFFER_H
#define STORAGEBUFFER_H

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include "util/vkutil.h"

class StorageBuffer
{
    public:
        StorageBuffer(const vkutil::VulkanState & state, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
        virtual ~StorageBuffer();

        static void createBuffer(const VmaAllocator & allocator, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer & buffer, VmaAllocation & memory);
        static uint32_t findMemoryType(uint32_t filter, VkMemoryPropertyFlags properties, const VkPhysicalDevice & physicalDevice);

        VkBuffer & getBuffer();
        VmaAllocation & getMemory();

        virtual void upload(const VkDevice & device, const VkCommandPool & commandPool, const VkQueue & q);

    protected:

        VkDevice device;

        VkBuffer buffer;
        VmaAllocation memory;

        const vkutil::VulkanState & state;

        VkDeviceSize bufferSize;
        VkBuffer stagingBuffer;
        VmaAllocation stagingBufferMemory;

    private:

};

#endif // STORAGEBUFFER_H
