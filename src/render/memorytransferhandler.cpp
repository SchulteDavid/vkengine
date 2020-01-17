#include "memorytransferhandler.h"

MemoryTransferer::MemoryTransferer(MemoryTransferHandler & handler) :handler(handler) {

}

LambdaTransferer::LambdaTransferer(MemoryTransferHandler & handler, std::function<void(VkCommandBuffer &)> func) : MemoryTransferer(handler) {

    this->func = func;

}

void LambdaTransferer::recordTransfer(VkCommandBuffer & buffer) {
    this->func(buffer);
}

bool LambdaTransferer::reusable() {
    return false;
}

MemoryTransferHandler::MemoryTransferHandler() {

}

MemoryTransferHandler::~MemoryTransferHandler() {

}

void MemoryTransferHandler::signalTransfer(MemoryTransferer * obj) {
    this->transfers.push(obj);
}

bool MemoryTransferHandler::hasPendingTransfer() {
    return !this->transfers.empty();
}

void MemoryTransferHandler::recordTransfer(VkCommandBuffer & buffer) {

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    vkBeginCommandBuffer(buffer, &beginInfo);

    while (!this->transfers.empty()) {

        MemoryTransferer * trans = transfers.front();
        transfers.pop();

        trans->recordTransfer(buffer);

        if (!trans->reusable())
            delete trans;

    }

    vkEndCommandBuffer(buffer);

}
