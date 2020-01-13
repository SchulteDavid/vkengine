#ifndef VERTEXBUFFER_H
#define VERTEXBUFFER_H

#include "storagebuffer.h"
#include <string.h>

template <typename T> class VertexBuffer : public StorageBuffer {

    public:
        VertexBuffer(const vkutil::VulkanState & state, const std::vector<T> & data) : StorageBuffer(state, sizeof(T) * data.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {

            bufferSize = sizeof(T) * data.size();

            VkBufferCreateInfo stBufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
            stBufferCreateInfo.size = bufferSize;
            stBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            stBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo stAllocCreateInfo = {};
            stAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
            stAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

            VmaAllocationInfo stagingBufferAllocInfo = {};

            vmaCreateBuffer(state.vmaAllocator, &stBufferCreateInfo, &stAllocCreateInfo, &stagingBuffer, &stagingBufferMemory, &stagingBufferAllocInfo);

            void * tmp;
            //vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &tmp);
            vmaMapMemory(state.vmaAllocator, stagingBufferMemory, &tmp);
            //memcpy(tmp, data.data(), bufferSize);
            memcpy(stagingBufferAllocInfo.pMappedData, data.data(), bufferSize);
            //vkUnmapMemory(device, stagingBufferMemory);
            vmaUnmapMemory(state.vmaAllocator, stagingBufferMemory);

        }
        virtual ~VertexBuffer(){

        }

        void bindForRender(VkCommandBuffer & cmdBuffer) {

            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &buffer, offsets);

        }

        void upload(const VkDevice & device, const VkCommandPool & commandPool, const VkQueue & q) {

            vkutil::copyBuffer(stagingBuffer, buffer, bufferSize, commandPool, device, q);
            vmaDestroyBuffer(state.vmaAllocator, stagingBuffer, stagingBufferMemory);

        }

    protected:

    private:

};

#endif // VERTEXBUFFER_H
