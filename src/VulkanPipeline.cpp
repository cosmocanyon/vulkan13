#include "VulkanPipeline.h"

#include "vk_initializers.h"
#include "VulkanModel.h"

#include <fstream>
#include <array>
#include <cassert>

using namespace moo;

VulkanPipeline::VulkanPipeline(VulkanDevice& device, const std::string& vertFilepath, const std::string& fragFilepath, const PipelineConfigInfo& configInfo) : m_device{device} {
    createGraphicsPipeline(vertFilepath, fragFilepath, configInfo);
}

VulkanPipeline::~VulkanPipeline() {
    vkDestroyShaderModule(m_device.getDevice(), m_vertShaderModule, nullptr);
    vkDestroyShaderModule(m_device.getDevice(), m_fragShaderModule, nullptr);
    vkDestroyPipeline(m_device.getDevice(), m_graphicsPipeline, nullptr);
}

std::vector<char> VulkanPipeline::readFile(const std::string& filepath) {
    // open file, cursor at the end
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);

    if(!file.is_open())
        throw std::runtime_error("Failed to open file: " + filepath);

    std::streampos filesize = file.tellg();
    // spirv expects the buffer to be an uint32 instead of a char,
    // reserve an int vector big enough for the entire file ELEMENTS
    // std::vector<uint32_t> buffer(filesize / sizeof(uint32_t)); // bytes -> uint32
    // std::vector<char> buffer(fileSize); equals to 
    std::vector<char> buffer(filesize / sizeof(char));

    file.seekg(0, std::ios::beg); // file cursor at the beginning, rdy to read 
    file.read(buffer.data(), filesize);

    file.close();
    return buffer;
}

void VulkanPipeline::createGraphicsPipeline(const std::string& vertFilepath, const std::string& fragFilepath, const PipelineConfigInfo& configInfo) {
    assert(configInfo.pipelineLayout != VK_NULL_HANDLE && "Cannot create graphics pipeline: no pipelineLayout provided to configInfo.");
    assert(configInfo.renderpass != VK_NULL_HANDLE && "Cannot create graphics pipeline: no renderpass provided to configInfo.");
    
    std::vector<char> vertCode = readFile(vertFilepath);
    std::vector<char> fragCode = readFile(fragFilepath);

    createShaderModule(vertCode, &m_vertShaderModule, vertFilepath);
    createShaderModule(fragCode, &m_fragShaderModule, fragFilepath);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, m_vertShaderModule);
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, m_fragShaderModule);

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

    // vertex input
    auto binding_descriptions = VulkanModel::Vertex::getBindingDescriptions();
    auto attributes_descriptions = VulkanModel::Vertex::getAttributeDescriptions();
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkinit::pipelineVertexInputCreateInfo();
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(binding_descriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = binding_descriptions.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes_descriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributes_descriptions.data();

    VkPipelineColorBlendStateCreateInfo colorBlending {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &configInfo.colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    VkGraphicsPipelineCreateInfo pipelineInfo {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
// Shader stages: the shader modules that define the functionality of the programmable stages of the graphics pipeline
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();

// Fixed-function state: all of the structures that define the fixed-function stages of the pipeline, like input assembly, rasterizer, viewport and color blending
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
    pipelineInfo.pViewportState = &configInfo.viewportInfo;
    pipelineInfo.pRasterizationState = &configInfo.rasterizerInfo;
    pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
    //pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo; // Optional
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &configInfo.dynamicStateInfo;

// Pipeline layout: the uniform and push values referenced by the shader that can be updated at draw time
    pipelineInfo.layout = configInfo.pipelineLayout;
// Render pass: the attachments referenced by the pipeline stages and their usage
    pipelineInfo.renderPass = configInfo.renderpass;
    pipelineInfo.subpass = configInfo.subpass;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    if(vkCreateGraphicsPipelines(m_device.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline."); // failed to create graphics pipeline
    }
}

void VulkanPipeline::createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule, const std::string& filepath) {
    VkShaderModuleCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = code.size();
    info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    // check creation went well, 
    // cause there are often error VK_CHECK isn't suitable
    if(vkCreateShaderModule(m_device.getDevice(), &info, nullptr, shaderModule) != VK_SUCCESS)
        throw std::runtime_error("Failed to create shader module: " + filepath);
}

void VulkanPipeline::defaultPipelineConfigInfo(PipelineConfigInfo &configInfo) {
    configInfo.inputAssemblyInfo = vkinit::pipeInputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    configInfo.viewportInfo = vkinit::pipelineViewportStateInfo();

    configInfo.rasterizerInfo = vkinit::rasterizationStageCreateInfo(VK_POLYGON_MODE_FILL);

    configInfo.multisampleInfo = vkinit::multisamplingCreateInfo();

    // color blending configuration per attached framebuffer
    configInfo.colorBlendAttachment = vkinit::colorblendAttachmentState();

    //configInfo.depthStencilInfo = vkinit::depthStencilCreateInfo();

    configInfo.dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    configInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    configInfo.dynamicStateInfo.pNext = nullptr;
    configInfo.dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
    configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();
    configInfo.dynamicStateInfo.flags = 0;
}

void VulkanPipeline::bind(VkCommandBuffer commandBuffer) {
    assert(m_graphicsPipeline != nullptr);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
}