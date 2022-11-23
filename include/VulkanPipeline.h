#pragma once

#include "VulkanDevice.h"

#include <string>

namespace moo {

class VulkanDevice;

class VulkanPipeline {
public:
    struct PipelineConfigInfo {
        PipelineConfigInfo(const PipelineConfigInfo&) = delete;
        PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

        VkPipelineViewportStateCreateInfo viewportInfo;
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        VkPipelineRasterizationStateCreateInfo rasterizerInfo;
        VkPipelineMultisampleStateCreateInfo multisampleInfo;
        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo;

        std::vector<VkDynamicState> dynamicStateEnables;
        VkPipelineDynamicStateCreateInfo dynamicStateInfo;

        VkPipelineLayout pipelineLayout = nullptr;
        VkRenderPass renderpass = nullptr;
        uint32_t subpass = 0;
    };

private:
    VulkanDevice& m_device;

    VkPipeline m_graphicsPipeline;
    VkShaderModule m_vertShaderModule, m_fragShaderModule;

public:
    VulkanPipeline(VulkanDevice &device, const std::string &vertFilepath, const std::string &fragFilepath, const PipelineConfigInfo &configInfo);
    ~VulkanPipeline();

    VulkanPipeline(const VulkanPipeline&) = delete;
    VulkanPipeline& operator=(const VulkanPipeline&) = delete;

    static void defaultPipelineConfigInfo(PipelineConfigInfo &configInfo);

    void bind(VkCommandBuffer commandBuffer);
    
private:
    static std::vector<char> readFile(const std::string &filepath);
    void createGraphicsPipeline(const std::string &vertFilepath, const std::string &fragFilepath, const PipelineConfigInfo &configInfo);
    void createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule, const std::string &filepath);
};

}   // namespace moo