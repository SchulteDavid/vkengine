#include "storagebuffer.h"
#include <stdexcept>
#include "util/vkutil.h"

#include "util/debug/trace_exception.h"

StorageBuffer::StorageBuffer(const vkutil::VulkanState & state, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) : state(state) {

    createBuffer(state.vmaAllocator, size, usage, properties, buffer, memory);
    //this->device = VulkanHelper::getDevice();

}

StorageBuffer::~StorageBuffer() {

    vmaDestroyBuffer(state.vmaAllocator, buffer, memory);
}

uint32_t StorageBuffer::findMemoryType(uint32_t filter, VkMemoryPropertyFlags properties, const VkPhysicalDevice & physicalDevice) {

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {

        if ((filter & (1 << i)) && ((memProperties.memoryTypes[i].propertyFlags & properties) == properties))
            return i;

    }

    throw dbg::trace_exception("No suitable memory found");

}

VkBuffer & StorageBuffer::getBuffer() {
    return buffer;
}

VmaAllocation & StorageBuffer::getMemory() {
    return memory;
}

void StorageBuffer::createBuffer(const VmaAllocator & allocator, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer & buffer, VmaAllocation & memory) {

    //VkDevice device = VulkanHelper::getDevice();

    if (properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        throw dbg::trace_exception("Bad buffer alloc");

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    //allocInfo.requiredFlags = properties;

    vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &memory, nullptr);

    /*if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        throw dbg::trace_exception("Unable to create vertex buffer");


    VkMemoryRequirements memRequirements = {};
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
        throw dbg::trace_exception("Unable to allocate memory");

    vkBindBufferMemory(device, buffer, memory, 0);*/

}

void StorageBuffer::upload(const VkDevice & device, const VkCommandPool & commandPool, const VkQueue & q) {

}
