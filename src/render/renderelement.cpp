#include "renderelement.h"

#include "storagebuffer.h"

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <string.h>
#include "viewport.h"

RenderElement::RenderElement(Viewport * view, std::shared_ptr<Model> model, std::shared_ptr<Shader> shader, std::vector<std::shared_ptr<Texture>> texture, int scSize, Transform & initTransform) : state(view->getState()), MemoryTransferer(*view) {

    //throw std::runtime_error("Created RenderElement with old constructor");

    this->model = model;
    this->shader = shader;
    this->texture = texture;

    instanceTransforms = std::vector<glm::mat4>(1);
    instances = std::unordered_map<uint32_t, InstanceInfo>(1);
    transforms = std::vector<Transform>(1);

    std::array<float, 3> rAxis = {0.0, 0.0, 1.0};

    transforms[0] = initTransform;

    instanceTransforms[0] = getTransformationMatrix(transforms[0]);

    instanceCount = 1;

    this->instanceBuffer = new DynamicBuffer<glm::mat4>(state, instanceTransforms, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    this->createUniformBuffers(scSize);

    this->descPool = this->shader->setupDescriptorPool(scSize);
    this->descriptorSets = shader->createDescriptorSets(descPool, descSetLayout, uniformBuffers, sizeof(UniformBufferObject), texture, scSize);

}

RenderElement::RenderElement(Viewport * view, std::shared_ptr<Model> model, std::shared_ptr<Material> mat, int scSize, Transform & initTransform) : state(view->getState()), MemoryTransferer(*view) {

    this->model = model;
    this->shader = mat->getShader();
    this->texture = mat->getTextures();

    instanceTransforms = std::vector<glm::mat4>(1);
    instances = std::unordered_map<uint32_t, InstanceInfo>(1);
    transforms = std::vector<Transform>(1);

    std::array<float, 3> rAxis = {0.0, 0.0, 1.0};

    transforms[0] = initTransform;

    instanceTransforms[0] = getTransformationMatrix(transforms[0]);

    instanceCount = 1;

    this->instanceBuffer = new DynamicBuffer<glm::mat4>(state, instanceTransforms, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    this->createUniformBuffers(scSize);

    Shader::Binding uniformBufferBinding;
    uniformBufferBinding.type = Shader::BINDING_UNIFORM_BUFFER;
    uniformBufferBinding.elementCount = 1;
    uniformBufferBinding.bindingId = 0;

    Shader::Binding textureBinding;
    textureBinding.type = Shader::BINDING_TEXTURE_SAMPLER;
    textureBinding.bindingId = 1;

    std::vector<Shader::Binding> binds(2);
    binds[0] = uniformBufferBinding;
    binds[1] = textureBinding;

    descSetLayout = mat->prepareDescriptors(binds);
    pipeline = mat->setupPipeline(state, view->getRenderpass(), view->getSwapchainExtent(), descSetLayout, model.get(), pipelineLayout);

    this->descPool = this->shader->setupDescriptorPool(scSize);
    this->descriptorSets = shader->createDescriptorSets(descPool, descSetLayout, uniformBuffers, sizeof(UniformBufferObject), texture, scSize);

}

RenderElement::RenderElement(Viewport * view, std::shared_ptr<Structure> strc, Transform & initTransform) : RenderElement(view, strc->getModel(), strc->getMaterial(), view->getSwapchainSize(), initTransform) {



}

RenderElement::~RenderElement() {

    //destroyUniformBuffers();
    vkDestroyDescriptorPool(state.device, descPool, nullptr);

    delete this->instanceBuffer;

}

glm::mat4 RenderElement::toGLMMatrx(Math::Matrix<4,4,float> m) {

    glm::mat4 mat = glm::mat4(1.0);
    mat[0] = glm::vec4(m(0,0), m(1,0), m(2,0), m(3,0));
    mat[1] = glm::vec4(m(0,1), m(1,1), m(2,1), m(3,1));
    mat[2] = glm::vec4(m(0,2), m(1,2), m(2,2), m(3,2));
    mat[3] = glm::vec4(m(0,3), m(1,3), m(2,3), m(3,3));

    return mat;

}

glm::mat4 RenderElement::getTransformationMatrix(Transform & i) {

    glm::mat4 smat = glm::scale(glm::vec3(i.scale, i.scale, i.scale));

    //Math::Matrix<4,4,float> smat = Math::scaleMatrix<4,4,float>(i.scale);

    glm::mat4 trmat = toGLMMatrx(i.qRot.toModelMatrix(i.position));
    //Math::Matrix<4,4,float> trmat = i.qRot.toModelMatrix(i.position);

    return trmat * smat;//.toGlmMatrix();

}

void RenderElement::markBufferDirty() {
    this->instanceBufferDirty = true;
    this->handler.signalTransfer(this);
}

RenderElement::Instance RenderElement::addInstance(Transform & i) {

    uint32_t index = this->transforms.size();

    transforms.push_back(i);
    instanceTransforms.push_back(getTransformationMatrix(i));
    //instances.push_back((InstanceInfo){index, index});
    instances[index] = (InstanceInfo){index, index};
    this->markBufferDirty();

    this->instanceBuffer->recreate(instanceTransforms);

    instanceCount++;
    this->instanceCountUpdated = true;

    return (Instance) {index};

}

void RenderElement::updateInstance(Instance & instance, Transform & trans) {

    //Check for existance of instance.
    if (this->instances.find(instance.id) == instances.end())
        return;

    InstanceInfo info = this->instances[instance.id];

    transformBufferMutex.lock();

    this->transforms[info.pos] = trans;
    this->instanceTransforms[info.pos] = getTransformationMatrix(transforms[info.pos]);

    transformBufferMutex.unlock();

    this->markBufferDirty();

}

void RenderElement::deleteInstance(Instance & instance) {

    transformBufferMutex.lock();

    ///No instances left
    if (!transforms.size()) {
        return;
    }

    //Info of instance to remove
    InstanceInfo info = this->instances[instance.id];
    uint32_t lastPos = instanceCount - 1;

    //remove instance from map
    this->instances.erase(this->instances.find(instance.id));

    //getting info for last element
    InstanceInfo * lastInfo;
    for (auto it : instances) {

        if (it.second.pos == lastPos) {
            lastInfo = &it.second;
            break;
        }

    }

    //Moving last element to override deleted one.
    transforms[info.pos] = transforms[lastPos];
    instanceTransforms[info.pos] = instanceTransforms[lastPos];
    lastInfo->pos = info.pos;

    this->instanceBuffer->recreate(this->instanceTransforms);
    this->markBufferDirty();

    instanceCount--;
    //this->instanceCountUpdated = true;
    transformBufferMutex.unlock();

}

void RenderElement::createUniformBuffers(int swapChainSize) {

    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(swapChainSize);
    uniformBuffersMemory.resize(swapChainSize);

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

    }

}

void RenderElement::recreateResources(VkRenderPass & renderPass, int scSize, const vkutil::SwapChain & swapchain) {

    vkutil::VertexInputDescriptions descs;
    descs.attributes = Model::Vertex::getAttributeDescriptions();
    descs.binding = Model::Vertex::getBindingDescription();

    pipeline = shader->setupGraphicsPipeline(descs, renderPass, state, descSetLayout, swapchain.extent, pipelineLayout);

    descPool = shader->setupDescriptorPool(scSize);
    createUniformBuffers(scSize);
    this->descriptorSets = shader->createDescriptorSets(descPool, descSetLayout, uniformBuffers, sizeof(UniformBufferObject), texture, scSize);

}

void RenderElement::destroyUniformBuffers(const vkutil::SwapChain & swapchain) {

    for (int i = 0; i < swapchain.images.size(); ++i) {

        vmaDestroyBuffer(state.vmaAllocator, uniformBuffers[i], uniformBuffersMemory[i]);

    }

}

void RenderElement::recordTransfer(VkCommandBuffer & cmdBuffer) {

    transformBufferMutex.lock();
    this->instanceBuffer->fill(instanceTransforms, cmdBuffer);
    transformBufferMutex.unlock();

}

bool RenderElement::reusable() {
    return true;
}

void RenderElement::updateUniformBuffer(UniformBufferObject & obj,  uint32_t imageIndex) {

    void * data;
    vmaMapMemory(state.vmaAllocator, uniformBuffersMemory[imageIndex], &data);
    memcpy(data, &obj, sizeof(UniformBufferObject));
    vmaUnmapMemory(state.vmaAllocator, uniformBuffersMemory[imageIndex]);

    if (this->instanceCountUpdated) {

        this->instanceCountUpdated = false;

    }
    if (this->instanceBufferDirty) {

        this->instanceBufferDirty = false;

    }

}

bool RenderElement::needsDrawCmdUpdate() {
    return instanceCountUpdated;
}

Shader * RenderElement::getShader() {
    return this->shader.get();
}

void RenderElement::render(VkCommandBuffer & cmdBuffer, uint32_t frameIndex) {

    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[frameIndex], 0, nullptr);

    model->bindForRender(cmdBuffer);
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmdBuffer, 1, 1, &instanceBuffer->getBuffer(), offsets);

    vkCmdDrawIndexed(cmdBuffer, model->getIndexCount(), instanceCount, 0, 0, 0);

}

void RenderElement::renderShaderless(VkCommandBuffer & buffer, uint32_t frameIndex) {

    vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[frameIndex], 0, nullptr);

    model->bindForRender(buffer);
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(buffer, 1, 1, &instanceBuffer->getBuffer(), offsets);
    vkCmdDrawIndexed(buffer, model->getIndexCount(), instanceCount, 0, 0, 0);

}

std::vector<VkDescriptorSet> & RenderElement::getDescriptorSets() {
    return this->descriptorSets;
}

std::vector<VmaAllocation> & RenderElement::getMemories() {
    return this->uniformBuffersMemory;
}

RenderElement * RenderElement::buildRenderElement(Viewport * view, std::shared_ptr<Structure> strc, RenderElement::Transform & initTrans) {

    return new RenderElement(view, strc, initTrans);

}
