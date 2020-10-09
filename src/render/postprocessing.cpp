#include "render/postprocessing.h"

#include "util/debug/logger.h"
#include "render/util/vk_trace_exception.h"

PPEffect::PPEffect(std::shared_ptr<Shader> shader) {
  this->shader = shader;
}

PPEffect::~PPEffect() {

}

void PPEffect::bindForRender(VkCommandBuffer &buffer) {

  vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  /// TODO push descriptors if any are required
  
}

void PPEffect::setupPipeline(const vkutil::VulkanState & state, const vkutil::SwapChain & swapchain, VkRenderPass renderPass, uint32_t subpass) {
  
  lout << "Creating post-processing pipeline" << std::endl;

  /// Get shader info in useful format
  std::vector<vkutil::ShaderInputDescription> shaders = shader->getShaderInputDescriptions();
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages(shaders.size());
  for (unsigned int i = 0; i < shaders.size(); ++i) {
    
    shaderStages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[i].module = shaders[i].module;
    shaderStages[i].stage = shaders[i].usage;
    shaderStages[i].pName = shaders[i].entryName;

  }

  /// Configure fixed functions
  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  VkVertexInputBindingDescription description = {};
  description.binding = 0;
  description.stride = sizeof(Model::Vertex);
  description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  std::vector<VkVertexInputAttributeDescription> descriptions(3);

  descriptions[0].binding = 0;
  descriptions[0].location = 0;
  descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  descriptions[0].offset = offsetof(Model::Vertex, pos);

  descriptions[1].binding = 0;
  descriptions[1].location = 1;
  descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  descriptions[1].offset = offsetof(Model::Vertex, normal);

  descriptions[2].binding = 0;
  descriptions[2].location = 2;
  descriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
  descriptions[2].offset = offsetof(Model::Vertex, tangent);

  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &description;
  vertexInputInfo.vertexAttributeDescriptionCount = descriptions.size();
  vertexInputInfo.pVertexAttributeDescriptions = descriptions.data();

  vkutil::VertexInputDescriptions inputDescriptions;
  inputDescriptions.binding = {description};
  inputDescriptions.attributes = descriptions;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  /// Configure the viewport
  VkViewport viewport = {};
  viewport.x = 0;
  viewport.y = 0;
  viewport.width = swapchain.extent.width;
  viewport.height = swapchain.extent.height;
  viewport.minDepth = 0.0;
  viewport.maxDepth = 1.0;

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = swapchain.extent;

  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  /* Rasterizer */

  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0;
  rasterizer.cullMode = VK_CULL_MODE_NONE;
  //rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0;
  rasterizer.depthBiasClamp = 0.0;
  rasterizer.depthBiasSlopeFactor = 0.0;

  /* Multisampling */

  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f;
  multisampling.pSampleMask = nullptr;
  multisampling.alphaToCoverageEnable = VK_FALSE;
  multisampling.alphaToOneEnable = VK_FALSE;

  /* color blending */

  // For one framebuffer
  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  // global configuration
  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  VkPipelineDepthStencilStateCreateInfo depthStencil = {};
  depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.minDepthBounds = 0.0f;
  depthStencil.maxDepthBounds = 1.0f;
  depthStencil.stencilTestEnable = VK_FALSE;
  depthStencil.front = {};
  depthStencil.back = {};

  /* Uniform layout */


  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &descriptorLayout; ///Get from Shader object
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = nullptr;

  if (VkResult res = vkCreatePipelineLayout(state.device, &pipelineLayoutInfo, nullptr, &pipelineLayout))
    throw vkutil::vk_trace_exception("Unable to create pipeline layout", res);


  /** Creating the pipeline **/

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = shaderStages.size();
  pipelineInfo.pStages = shaderStages.data();
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = nullptr; // change for dynamic state
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = subpass;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex = -1;

  if (VkResult res = vkCreateGraphicsPipelines(state.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline))
    throw vkutil::vk_trace_exception("Unable to create pipeline", res);
  
}
