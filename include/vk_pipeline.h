#pragma once

#include <vector>

#include "vk_types.h"

namespace mii {

class PipelineBuilder {
public:
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    VkPipelineViewportStateCreateInfo viewportStateInfo;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineMultisampleStateCreateInfo multisampling;
    VkPipelineDepthStencilStateCreateInfo depthStencil;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineDynamicStateCreateInfo dynamicStateInfo;
    VkPipelineLayout pipelineLayout;

public:
    VkPipeline build_pipeline(VkDevice device, VkRenderPass renderpass);
};

}   // namespace mii