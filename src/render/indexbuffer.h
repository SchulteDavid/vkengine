#ifndef INDEXBUFFER_H
#define INDEXBUFFER_H

#include "storagebuffer.h"
#include "render/util/vkutil.h"
#include <type_traits>

#include <vector>
#include <string.h>

template <typename T = uint16_t> class IndexBuffer : public StorageBuffer {

    public:
        IndexBuffer(const vkutil::VulkanState & state, const std::vector<T> & indices) :
         StorageBuffer(state, sizeof(T) * indices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {

            bufferSize = sizeof(indices[0]) * indices.size();

            VkBufferCreateInfo stBufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
            stBufferCreateInfo.size = bufferSize;
            stBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            stBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo stAllocCreateInfo = {};
            stAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
            stAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

            VmaAllocationInfo stagingBufferAllocInfo = {};

            vmaCreateBuffer(state.vmaAllocator, &stBufferCreateInfo, &stAllocCreateInfo, &stagingBuffer, &stagingBufferMemory, &stagingBufferAllocInfo);

            void * data;
            //vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);

            vmaMapMemory(state.vmaAllocator, stagingBufferMemory, &data);
            memcpy(stagingBufferAllocInfo.pMappedData, indices.data(), bufferSize);
            vmaUnmapMemory(state.vmaAllocator, stagingBufferMemory);

            //vkUnmapMemory(device, stagingBufferMemory);
            /*vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingBufferMemory, nullptr);*/

        }
        virtual ~IndexBuffer() {

        }

        void bindForRender(VkCommandBuffer & cmdBuffer) {

            vkCmdBindIndexBuffer(cmdBuffer, buffer, 0, (VkIndexType) (sizeof(T) / 4));

        }

        void upload(const VkDevice & device, const VkCommandPool & commandPool, const VkQueue & q) {

            vkutil::copyBuffer(stagingBuffer, buffer, bufferSize, commandPool, device, q);
            vmaDestroyBuffer(state.vmaAllocator, stagingBuffer, stagingBufferMemory);

        }

    protected:

    private:

};

#endif // INDEXBUFFER_H
