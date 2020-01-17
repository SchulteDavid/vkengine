#ifndef MEMORYTRANSFERHANDLER_H
#define MEMORYTRANSFERHANDLER_H

#include <queue>
#include <functional>
#include <vulkan/vulkan.h>

class MemoryTransferHandler;

class MemoryTransferer {

    public:

        MemoryTransferer(MemoryTransferHandler & handler);

        /**
        function called to execute the transfer of data.
        This is where commands like vkCmdCopyBuffer should be used.
        **/
        virtual void recordTransfer(VkCommandBuffer & cmdBuffer) = 0;

        /**
        Tells the Handler whether or not to keep this in memory after transfer
        true indicates the object is still in use and should not be deleted.
        **/
        virtual bool reusable() = 0;

    protected:
        MemoryTransferHandler & handler;



};

class LambdaTransferer : public MemoryTransferer {

    public:
        LambdaTransferer(MemoryTransferHandler & handler, std::function<void(VkCommandBuffer&)> transferFunction);

        void recordTransfer(VkCommandBuffer & buffer);
        bool reusable();

    private:

        std::function<void(VkCommandBuffer &)> func;

};

class MemoryTransferHandler
{
    public:
        MemoryTransferHandler();
        virtual ~MemoryTransferHandler();

        void signalTransfer(MemoryTransferer * obj);

        bool hasPendingTransfer();

        void recordTransfer(VkCommandBuffer & buffer);

    protected:

    private:

        std::queue<MemoryTransferer *> transfers;

};

#endif // MEMORYTRANSFERHANDLER_H
