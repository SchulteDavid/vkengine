#include "renderelementanim.h"

RenderElementAnim::RenderElementAnim(Viewport * view, std::shared_ptr<Structure> strc, Transform & initTransform) : RenderElement(view, strc, initTransform, getShaderBindings(strc->getMaterial())) {

    this->skin = strc->getSkin();

}

RenderElementAnim::~RenderElementAnim() {
    //dtor
}

std::vector<Shader::Binding> RenderElementAnim::getShaderBindings(std::shared_ptr<Material> material) {
  
  std::vector<Shader::Binding> binds = material->getDefaultBindings();

    Shader::Binding boneDataBinding;
    boneDataBinding.type = Shader::BINDING_UNIFORM_BUFFER;
    boneDataBinding.elementCount = 1;
    boneDataBinding.bindingId = binds.size();
    boneDataBinding.elementSize = 16 * sizeof(float);

    binds.push_back(boneDataBinding);

    return binds;

}

void RenderElementAnim::createUniformBuffers(int swapChainSize, std::vector<Shader::Binding> & bindings) {

    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    VkDeviceSize animationSize = this->skin->getDataSize();

    std::cout << "animationSize " << animationSize << " bufferSize " << bufferSize << std::endl;

    uniformBuffers.resize(swapChainSize);
    uniformBuffersMemory.resize(swapChainSize);

    animationBuffers.resize(swapChainSize);
    animationBuffersMemory.resize(swapChainSize);

    for (int i = 0; i < swapChainSize; ++i) {

        VkBufferCreateInfo stBufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        stBufferCreateInfo.size = bufferSize;
        stBufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        stBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo stAllocCreateInfo = {};
        stAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        stAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo stagingBufferAllocInfo = {};

        vmaCreateBuffer(state.vmaAllocator, &stBufferCreateInfo, &stAllocCreateInfo, &uniformBuffers[i], &uniformBuffersMemory[i], &stagingBufferAllocInfo);

        VkBufferCreateInfo stAnimBufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        stAnimBufferCreateInfo.size = animationSize;
        stAnimBufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        stAnimBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo stAnimAllocCreateInfo = {};
        stAnimAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        stAnimAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo stagingAnimBufferAllocInfo = {};

        vmaCreateBuffer(state.vmaAllocator, &stAnimBufferCreateInfo, &stAnimAllocCreateInfo, &animationBuffers[i], &animationBuffersMemory[i], &stagingAnimBufferAllocInfo);

    }

    bindings[0].uniformBuffers = uniformBuffers;
    std::cout << "Setting animation Buffer" << std::endl;
    bindings[bindings.size()-1].uniformBuffers = animationBuffers;

}

void RenderElementAnim::destroyUniformBuffers(const vkutil::SwapChain & swapchain) {

    for (int i = 0; i < swapchain.images.size(); ++i) {

        vmaDestroyBuffer(state.vmaAllocator, uniformBuffers[i], uniformBuffersMemory[i]);
        vmaDestroyBuffer(state.vmaAllocator, animationBuffers[i], animationBuffersMemory[i]);

    }

    /// TODO: Destroy animation data buffers.

}

void RenderElementAnim::setSkin(std::shared_ptr<Skin> skin) {
    this->skin = skin;
}

void RenderElementAnim::updateUniformBuffer(UniformBufferObject & obj, uint32_t imageIndex) {

    void * data;
    vmaMapMemory(state.vmaAllocator, uniformBuffersMemory[imageIndex], &data);
    memcpy(data, &obj, sizeof(UniformBufferObject));
    vmaUnmapMemory(state.vmaAllocator, uniformBuffersMemory[imageIndex]);

    vmaMapMemory(state.vmaAllocator, animationBuffersMemory[imageIndex], &data);
    this->skin->writeTransformDataToBuffer((float *) data);
    vmaUnmapMemory(state.vmaAllocator, animationBuffersMemory[imageIndex]);

    if (this->instanceCountUpdated) {

        this->instanceCountUpdated = false;

    }
    if (this->instanceBufferDirty) {

        this->instanceBufferDirty = false;

    }

}
