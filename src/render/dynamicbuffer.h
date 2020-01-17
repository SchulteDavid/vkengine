#ifndef DYNAMICBUFFER_H
#define DYNAMICBUFFER_H

#include <vector>

#include "storagebuffer.h"
#include "util/vkutil.h"


template <typename T> class DynamicBuffer : public StorageBuffer
{
    public:
        DynamicBuffer(const vkutil::VulkanState & state, std::vector<T> & data, VkBufferUsageFlags usage) : StorageBuffer(state, sizeof(T) * data.size(), usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {

            bufferSize = sizeof(T) * data.size();

            this->usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

            createStagingBuffer();
            fill(state, data);

        }

        virtual ~DynamicBuffer() {

            destroyStagingBuffer();

        }

        void fill(const vkutil::VulkanState & state, std::vector<T> & data) {

            void * tmp;
            vmaMapMemory(state.vmaAllocator, stagingBufferMemory, &tmp);
            memcpy(tmp, data.data(), bufferSize);
            //vmaFlushAllocation(allocator, stagingBufferMemory, 0, bufferSize);
            vkutil::copyBuffer(stagingBuffer, buffer, bufferSize, state.transferCommandPool, state.device, state.transferQueue);
            vmaUnmapMemory(state.vmaAllocator, stagingBufferMemory);

        }

        void fill(std::vector<T> & data, VkCommandBuffer & cmdBuffer) {

            void * tmp;
            vmaMapMemory(state.vmaAllocator, stagingBufferMemory, &tmp);
            memcpy(tmp, data.data(), bufferSize);

            VkBufferCopy copyRegion = {};
            copyRegion.srcOffset = 0;
            copyRegion.dstOffset = 0;
            copyRegion.size = bufferSize;

            vkCmdCopyBuffer(cmdBuffer, stagingBuffer, buffer, 1, &copyRegion);

            vmaUnmapMemory(state.vmaAllocator, stagingBufferMemory);

        }

        void updateElement(uint32_t index, T * newData) {

            void * tmp;
            vmaMapMemory(state.vmaAllocator, stagingBufferMemory, &tmp);
            memcpy(((T *)tmp) + index, newData, sizeof(T));
            vkutil::copyBuffer(stagingBuffer, buffer, bufferSize, state.transferCommandPool, state.device, state.transferQueue);
            vmaUnmapMemory(state.vmaAllocator, stagingBufferMemory);

        }

        void recreate(std::vector<T> & data) {


            destroyDataBuffer();
            destroyStagingBuffer();

            bufferSize = sizeof(T) * data.size();

            createStagingBuffer();
            createBuffer(state.vmaAllocator, bufferSize, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, memory);

            fill(state, data);

        }



    protected:

    private:


        VkBuffer stagingBuffer;
        VmaAllocation stagingBufferMemory;
        VkDeviceSize bufferSize;

        VkBufferUsageFlags usage;


        void destroyStagingBuffer() {

            vmaDestroyBuffer(state.vmaAllocator, stagingBuffer, stagingBufferMemory);

        }

        void destroyDataBuffer() {

            vmaDestroyBuffer(state.vmaAllocator, buffer, memory);

        }

        void createStagingBuffer() {

            VkBufferCreateInfo stBufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
            stBufferCreateInfo.size = bufferSize;
            stBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            stBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo stAllocCreateInfo = {};
            stAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
            stAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

            VmaAllocationInfo stagingBufferAllocInfo = {};

            vmaCreateBuffer(state.vmaAllocator, &stBufferCreateInfo, &stAllocCreateInfo, &stagingBuffer, &stagingBufferMemory, &stagingBufferAllocInfo);

        }


};

#endif // DYNAMICBUFFER_H
