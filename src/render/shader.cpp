#include "shader.h"

#include <iostream>
//#include <configloader.h>

#include <configloading.h>

#include "model.h"

#include "util/debug/trace_exception.h"
#include "util/vk_trace_exception.h"

Shader::Shader(std::string vertShader, std::string fragShader, const VkDevice & device) : device(device) {

  this->modules.resize(2);
  this->stages = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

  vertShaderCode = readFile(vertShader);
  fragShaderCode = readFile(fragShader);

  modules[0] = vkutil::createShaderModule(vertShaderCode, device);
  modules[1] = vkutil::createShaderModule(fragShaderCode, device);

  hasDescSets = false;

}

Shader::Shader(std::vector<uint8_t> vertCode, std::vector<uint8_t> fragCode, const vkutil::VulkanState & state, unsigned int textureSlots) : device(state.device) {

  vertShaderCode = vertCode;
  fragShaderCode = fragCode;

  hasDescSets = false;
  this->textureSlots = textureSlots;

}

Shader::~Shader() {

  for (unsigned int i = 0; i < modules.size(); ++i)
    vkDestroyShaderModule(device, modules[i], nullptr);

}

void Shader::createModules(const vkutil::VulkanState & state) {

  modules.resize(2);
  stages = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

  modules[0] = vkutil::createShaderModule(vertShaderCode, state.device);
  modules[1] = vkutil::createShaderModule(fragShaderCode, state.device);

}

std::vector<VkDescriptorSetLayoutBinding> Shader::getVkBindings(std::vector<Shader::Binding> bindings) {

  std::vector<VkDescriptorSetLayoutBinding> layoutBindings(bindings.size());

  for (unsigned int i = 0; i < bindings.size(); ++i) {

    VkDescriptorSetLayoutBinding b;
    b.binding = bindings[i].bindingId;
    b.pImmutableSamplers = 0;

    switch (bindings[i].type) {

    case BINDING_UNIFORM_BUFFER:
      b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      b.descriptorCount = bindings[i].elementCount;
      b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
      break;

    case BINDING_TEXTURE_SAMPLER:
      b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      b.descriptorCount = this->textureSlots;
      b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
      break;

    default:
      throw dbg::trace_exception(std::string("Wrong binding type specified for element ").append(std::to_string(i)).append(" : ").append(std::to_string(bindings[i].type)));

    }

    layoutBindings[i] = b;

  }

  return layoutBindings;

}

VkDescriptorSetLayout Shader::setupDescriptorSetLayout(std::vector<Shader::Binding> bindings) {

  std::vector<VkDescriptorSetLayoutBinding> binds = getVkBindings(bindings);
  //descSetLayout = vkutil::createDescriptorSetLayout(binds, device);
  return vkutil::createDescriptorSetLayout(binds, device);

}

void Shader::bindForRender(VkCommandBuffer & cmdBuffer, VkDescriptorSet & descriptors) {

  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptors, 0, nullptr);

}

VkPipeline Shader::setupGraphicsPipeline(vkutil::VertexInputDescriptions & descs, const VkRenderPass & renderPass, const vkutil::VulkanState & state, const VkDescriptorSetLayout & descLayout, VkExtent2D swapChainExtent, VkPipelineLayout & layout) {

  std::vector<vkutil::ShaderInputDescription> inputShaders(modules.size());

  std::string mainName = "main";

  for (unsigned int i = 0; i < modules.size(); ++i) {

    inputShaders[i].entryName = (char *) mainName.c_str();
    inputShaders[i].module = modules[i];
    inputShaders[i].usage = stages[i];

  }

  VkPipeline graphicsPipeline = vkutil::createGraphicsPipeline(state, renderPass, inputShaders, descs, descLayout, layout, swapChainExtent);

  this->graphicsPipeline = graphicsPipeline;

  this->pipelineLayout = layout;
  return graphicsPipeline;

}

VkDescriptorPool Shader::setupDescriptorPool(int scSize, std::vector<Binding> & binds) {

  std::vector<VkDescriptorPoolSize> sizes(binds.size());

  for (unsigned int i = 0; i < binds.size(); ++i) {

    Binding b = binds[i];
    VkDescriptorPoolSize poolSize;

    std::cout << "Creating poolSize for binding " << b.bindingId << " of type " << b.type << " and size " << b.elementCount << std::endl;
    
    switch (b.type) {

    case BINDING_UNIFORM_BUFFER:
      poolSize.descriptorCount = b.elementCount * scSize;
      poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      break;

    case BINDING_TEXTURE_SAMPLER:
      poolSize.descriptorCount = textureSlots * scSize;
      poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      break;

    }

    sizes[i] = poolSize;

  }

  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = sizes.size();
  poolInfo.pPoolSizes = sizes.data();
  poolInfo.maxSets = scSize;

  VkDescriptorPool descriptorPool;

  if (VkResult r = vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool))
    throw vkutil::vk_trace_exception("Unable to create descriptor pool", r);

  return descriptorPool;

}

/*std::vector<VkDescriptorSet> Shader::createDescriptorSets(VkDescriptorPool & descPool, const VkDescriptorSetLayout & descLayout, std::vector<VkBuffer> & uniformBuffers, size_t elementSize, std::vector<std::shared_ptr<Texture>> & tex, int scSize) {

  if (descPool == VK_NULL_HANDLE)
  throw dbg::trace_exception("Cannot create descriptor in NULL-pool!");

  std::cout << "Creating descriptor set with " << tex.size() << " textures" << std::endl;

  std::vector<VkDescriptorSetLayout> layouts(scSize, descLayout);

  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descPool;
  allocInfo.descriptorSetCount = scSize;
  allocInfo.pSetLayouts = layouts.data();

  this->descSets.resize(scSize);
  if (VkResult r = vkAllocateDescriptorSets(device, &allocInfo, descSets.data()))
  throw vkutil::vk_trace_exception("Unable to allocate descriptor sets", r);

  for (int i = 0; i < scSize; ++i) {

  int index = 0;

  std::vector<VkWriteDescriptorSet> descriptorWrites(2);

  VkDescriptorBufferInfo bufferInfo = {};
  bufferInfo.buffer = uniformBuffers[i];
  bufferInfo.offset = 0;
  bufferInfo.range = elementSize;

  descriptorWrites[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[index].dstSet = descSets[i];
  descriptorWrites[index].dstBinding = 0;
  descriptorWrites[index].dstArrayElement = 0;
  descriptorWrites[index].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorWrites[index].descriptorCount = 1;
  descriptorWrites[index].pBufferInfo = &bufferInfo;
  descriptorWrites[index].pImageInfo = nullptr;
  descriptorWrites[index].pTexelBufferView = nullptr;
  index++;

  std::vector<VkDescriptorImageInfo> imageInfos(textureSlots);

  for (unsigned int j = 0; j < tex.size(); ++j) {

  imageInfos[j].imageView = tex[j]->getView();
  imageInfos[j].sampler = tex[j]->getSampler();
  imageInfos[j].imageLayout = tex[j]->getLayout();

  }

  for (unsigned int j = tex.size(); j < textureSlots; ++j) {

  imageInfos[j].imageView = tex[0]->getView();
  imageInfos[j].sampler = tex[0]->getSampler();
  imageInfos[j].imageLayout = tex[0]->getLayout();

  }

  /// FOR

  descriptorWrites[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[index].dstSet = descSets[i];
  descriptorWrites[index].dstBinding = index;
  descriptorWrites[index].dstArrayElement = 0;
  descriptorWrites[index].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrites[index].descriptorCount = textureSlots;
  descriptorWrites[index].pBufferInfo = nullptr;
  descriptorWrites[index].pImageInfo = imageInfos.data();
  descriptorWrites[index].pTexelBufferView = nullptr;

  index++;

  /// END

  vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

  }

  return descSets;

  }*/

std::vector<VkDescriptorSet> Shader::createDescriptorSets(VkDescriptorPool & descPool, const VkDescriptorSetLayout & descLayout, std::vector<Binding> & binds, std::vector<std::shared_ptr<Texture>> & tex, int scSize) {

  std::cout << "Creating descriptor sets" << std::endl;

  if (descPool == VK_NULL_HANDLE)
    throw dbg::trace_exception("Cannot create descriptor in NULL-pool!");

  std::vector<VkDescriptorSet> descSets(scSize);
  std::vector<VkDescriptorSetLayout> layouts(scSize, descLayout);

  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descPool;
  allocInfo.descriptorSetCount = scSize;
  allocInfo.pSetLayouts = layouts.data();

  if (VkResult r = vkAllocateDescriptorSets(device, &allocInfo, descSets.data()))
    throw vkutil::vk_trace_exception("Unable to allocate descriptor sets", r);

  std::vector<VkDescriptorBufferInfo> bufferInfos(binds.size());

  for (int i = 0; i < scSize; ++i) {

    std::vector<VkWriteDescriptorSet> descriptorWrites(binds.size());
    std::vector<VkDescriptorImageInfo> imageInfos(textureSlots);

    for (unsigned int j = 0; j < binds.size(); ++j) {


      if (binds[j].type == BINDING_TEXTURE_SAMPLER) {

	for (unsigned int k = 0; k < tex.size(); ++k) {

	  imageInfos[k].imageView = tex[k]->getView();
	  imageInfos[k].sampler = tex[k]->getSampler();
	  imageInfos[k].imageLayout = tex[k]->getLayout();

	}

	for (unsigned int k = tex.size(); k < textureSlots; ++k) {

	  imageInfos[k].imageView = tex[0]->getView();
	  imageInfos[k].sampler = tex[0]->getSampler();
	  imageInfos[k].imageLayout = tex[0]->getLayout();

	}

	/// FOR

	descriptorWrites[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[j].dstSet = descSets[i];
	descriptorWrites[j].dstBinding = binds[j].bindingId;
	descriptorWrites[j].dstArrayElement = 0;
	descriptorWrites[j].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[j].descriptorCount = textureSlots;
	descriptorWrites[j].pBufferInfo = nullptr;
	descriptorWrites[j].pImageInfo = imageInfos.data();
	descriptorWrites[j].pTexelBufferView = nullptr;

	/// END

	continue;

      } else if (!binds[j].uniformBuffers.size())
	throw dbg::trace_exception(std::string("Empty uniform buffer in binding ").append(std::to_string(j)));
      else if (binds[j].uniformBuffers[i] == VK_NULL_HANDLE)
	throw dbg::trace_exception("Trying to use NULL-buffer as uniform buffer");

      bufferInfos[j].buffer = binds[j].uniformBuffers[i];
      bufferInfos[j].offset = 0;
      bufferInfos[j].range = binds[j].elementSize;


      descriptorWrites[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[j].dstSet = descSets[i];
      descriptorWrites[j].dstBinding = binds[j].bindingId;
      descriptorWrites[j].dstArrayElement = 0;
      descriptorWrites[j].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrites[j].descriptorCount = 1;
      descriptorWrites[j].pBufferInfo = &bufferInfos[j];
      descriptorWrites[j].pImageInfo = nullptr;
      descriptorWrites[j].pTexelBufferView = nullptr;

    }

    vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

  }

  return descSets;

}

VkPipeline & Shader::getPipeline() {
  return graphicsPipeline;
}

VkPipelineLayout & Shader::getPipelineLayout() {
  return this->pipelineLayout;
}

const std::vector<InputDescription> & Shader::getInputs() {
  return this->inputs;
}

void Shader::setInputs(std::vector<InputDescription> inputs) {
  this->inputs = inputs;
}

ShaderLoader::ShaderLoader(const vkutil::VulkanState & state) : state(state) {

}

std::shared_ptr<ResourceUploader<Shader>> ShaderLoader::loadResource(std::string fname) {

  /// TODO: add location information for texture bindings.

  using namespace config;

  std::shared_ptr<NodeCompound> tmpRoot = config::parseFile(fname);

  std::string vertFname(tmpRoot->getNode<char>("vertex")->getValueString());
  std::string fragFname(tmpRoot->getNode<char>("fragment")->getValueString());

  std::vector<uint8_t> vertCode = readFile(vertFname);
  std::vector<uint8_t> fragCode = readFile(fragFname);

  unsigned int textureCount = tmpRoot->getNode<int32_t>("textureCount")->getElement(0);

  std::shared_ptr<Shader> shader(new Shader(vertCode, fragCode, state, textureCount));

  if (tmpRoot->hasChild("inputs")) {

    std::shared_ptr<Node<std::shared_ptr<NodeCompound>>> elements = tmpRoot->getNode<std::shared_ptr<NodeCompound>>("inputs");

    uint32_t elemCount = elements->getElementCount();
    std::vector<InputDescription> inputs(elemCount);

    for (unsigned int i = 0; i < elemCount; ++i) {

      std::shared_ptr<NodeCompound> input = elements->getElement(i);

      std::string attributeName = input->getNode<char>("attributeName")->getValueString();
      int32_t location = input->getNode<int32_t>("location")->getElement(0);

      inputs[i].attributeName = attributeName;
      inputs[i].location = location;

    }

    shader->setInputs(inputs);


  } else {

    std::vector<InputDescription> vertElements(5);
    size_t  vertSize = sizeof(Model::Vertex);

    vertElements[0].attributeName = "POSITION";
    vertElements[0].location = 0;

    vertElements[1].attributeName = "NORMAL";
    vertElements[1].location = 12;

    vertElements[2].attributeName = "TANGENT";
    vertElements[2].location = 24;

    vertElements[3].attributeName = "TEXCOORD_0";
    vertElements[3].location = 36;

    vertElements[4].attributeName = "MATERIAL_INDEX";
    vertElements[4].location = 44;

    shader->setInputs(vertElements);

  }

  return std::shared_ptr<ShaderUploader>(new ShaderUploader(state, shader));

}

ShaderUploader::ShaderUploader(const vkutil::VulkanState & state, std::shared_ptr<Shader> shader) : state(state) {

  this->shader = shader;

}

std::shared_ptr<Shader> ShaderUploader::uploadResource() {

  this->shader->createModules(state);
  return shader;

}

bool ShaderUploader::uploadReady() {
  return true;
}

