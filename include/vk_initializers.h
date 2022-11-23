// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>

namespace vkinit {
	//vulkan init code goes here
	VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule);
	VkPipelineVertexInputStateCreateInfo pipelineVertexInputCreateInfo();
	VkPipelineInputAssemblyStateCreateInfo pipeInputAssemblyCreateInfo(VkPrimitiveTopology topology, VkBool32 primitive = VK_FALSE);
    VkPipelineRasterizationStateCreateInfo rasterizationStageCreateInfo(VkPolygonMode polygonMode);
	VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo();
	VkPipelineColorBlendAttachmentState colorblendAttachmentState();
	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo();
	VkPipelineViewportStateCreateInfo pipelineViewportStateInfo();

	// contains infos about shader inputs: push-constants, descriptor sets
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo();

	VkFramebufferCreateInfo framebufferCreateInfo(VkRenderPass renderpass, VkExtent2D extent);

	VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
    VkCommandBufferAllocateInfo commandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	VkCommandBufferBeginInfo commandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);
	VkRenderPassBeginInfo renderpassBeginInfo(VkRenderPass renderpass, VkExtent2D windowExtent, VkFramebuffer framebuffer);

	VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0);
    VkSemaphoreCreateInfo semaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);
	VkSubmitInfo submitInfo(const VkCommandBuffer* cmd);
	VkPresentInfoKHR presentInfo();
}