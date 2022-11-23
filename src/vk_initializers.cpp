#include <vk_initializers.h>

VkPipelineShaderStageCreateInfo vkinit::pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule) {
    VkPipelineShaderStageCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	info.pNext = nullptr;
	info.stage = stage; // shader stage
	info.module = shaderModule; // module containing code for this shader stage
	info.pName = "main"; // entry point of the shader, can be any funcion name: using "main" cause reflects the naming in the shaders written so far
	info.flags = 0;
	info.pSpecializationInfo = nullptr;

    return info;
}

VkPipelineVertexInputStateCreateInfo vkinit::pipelineVertexInputCreateInfo() {
	VkPipelineVertexInputStateCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	info.pNext = nullptr;
	// no vertex bindings or attributes
	info.vertexBindingDescriptionCount = 0;
	info.pVertexBindingDescriptions = nullptr; // Optional
	info.vertexAttributeDescriptionCount = 0;
	info.pVertexAttributeDescriptions = nullptr; // Optional
	
	return info;
}

VkPipelineInputAssemblyStateCreateInfo vkinit::pipeInputAssemblyCreateInfo(VkPrimitiveTopology topology, VkBool32 primitive) {
	VkPipelineInputAssemblyStateCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	info.pNext = nullptr;
	info.topology = topology;
	// not going to use primitive restart on the entire tutorial, leave it on false
	info.primitiveRestartEnable = primitive;

	return info;
}

VkPipelineRasterizationStateCreateInfo vkinit::rasterizationStageCreateInfo(VkPolygonMode polygonMode) {
	VkPipelineRasterizationStateCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	info.pNext = nullptr;

	info.depthClampEnable = VK_FALSE;
	// discard all primitives before rasterization stage if enabled, don't want it
	info.rasterizerDiscardEnable = VK_FALSE;

	info.polygonMode = polygonMode;
	info.lineWidth = 1.0f;
	info.cullMode = VK_CULL_MODE_NONE; // no backface cull
	//info.cullMode = VK_CULL_MODE_BACK_BIT;
	info.frontFace = VK_FRONT_FACE_CLOCKWISE;
	// no depth bias
	info.depthBiasEnable = VK_FALSE;
	info.depthBiasConstantFactor = 0.0f;
	info.depthBiasClamp = 0.0f;
	info.depthBiasSlopeFactor = 0.0f;

	return info;
}

VkPipelineMultisampleStateCreateInfo vkinit::multisamplingCreateInfo() {
	VkPipelineMultisampleStateCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	info.pNext = nullptr;

	info.sampleShadingEnable = VK_FALSE;
	info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // multisampling defaulted to no multisampling: 1 sample per pixel
	info.minSampleShading = 1.0f;
	info.pSampleMask = nullptr;
	info.alphaToCoverageEnable = VK_FALSE;
	info.alphaToOneEnable = VK_FALSE;

	return info;
}

VkPipelineColorBlendAttachmentState vkinit::colorblendAttachmentState() {
	VkPipelineColorBlendAttachmentState colorBlendAttachment {};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	return colorBlendAttachment;

	// alpha blending, where we want the new color to be blended with the old color based on its opacity
/*
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
*/	
}

VkPipelineDepthStencilStateCreateInfo vkinit::depthStencilCreateInfo() {
	VkPipelineDepthStencilStateCreateInfo info {};

	info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	info.pNext = nullptr;
	info.depthTestEnable = VK_TRUE;
	info.depthWriteEnable = VK_TRUE;
	info.depthCompareOp = VK_COMPARE_OP_LESS;
	info.depthBoundsTestEnable = VK_FALSE;
	info.minDepthBounds = 0.0f; // optional
	info.maxDepthBounds = 1.0f; // optional
	info.stencilTestEnable = VK_FALSE;
	info.front = {}; // optional
	info.back = {}; // optional

	return info;
}

VkPipelineLayoutCreateInfo vkinit::pipelineLayoutCreateInfo() {
	VkPipelineLayoutCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	info.pNext = nullptr;

	// empty defaults
	info.flags = 0;
	info.setLayoutCount = 0;
	info.pSetLayouts = nullptr;
	info.pushConstantRangeCount = 0;
	info.pPushConstantRanges = nullptr;
	
	return info;
}

VkPipelineViewportStateCreateInfo vkinit::pipelineViewportStateInfo() {
    VkPipelineViewportStateCreateInfo info {};

	info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    info.pNext = nullptr;
    info.viewportCount = 1;
    info.pViewports = nullptr;
    info.scissorCount = 1;
    info.pScissors = nullptr;

	return info;
}

VkFramebufferCreateInfo vkinit::framebufferCreateInfo(VkRenderPass renderpass, VkExtent2D extent) {
	VkFramebufferCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.pNext = nullptr;

    info.renderPass = renderpass;
    info.attachmentCount = 1;
    info.width = extent.width;
    info.height = extent.height;
    info.layers = 1;

	return info;
}

VkCommandPoolCreateInfo vkinit::commandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags /*= 0*/) {
    VkCommandPoolCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.pNext = nullptr;

	info.queueFamilyIndex = queueFamilyIndex;
	info.flags = flags;
	return info;
}

VkCommandBufferAllocateInfo vkinit::commandBufferAllocateInfo(VkCommandPool pool, uint32_t count /*= 1*/, VkCommandBufferLevel level /*= VK_COMMAND_BUFFER_LEVEL_PRIMARY*/) {
	VkCommandBufferAllocateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.pNext = nullptr;

	info.commandPool = pool;
	info.commandBufferCount = count;
	info.level = level;
	return info;
}

VkCommandBufferBeginInfo vkinit::commandBufferBeginInfo(VkCommandBufferUsageFlags flags /*= 0*/) {
	VkCommandBufferBeginInfo info {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.pNext = nullptr;

	info.pInheritanceInfo = nullptr;
	info.flags = flags;
	return info;
}

VkRenderPassBeginInfo vkinit::renderpassBeginInfo(VkRenderPass renderpass, VkExtent2D windowExtent, VkFramebuffer framebuffer) {
	VkRenderPassBeginInfo info {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.pNext = nullptr;

    info.renderPass = renderpass;
    info.renderArea.offset.x = 0;
    info.renderArea.offset.y = 0;
    info.renderArea.extent = windowExtent;
    info.framebuffer = framebuffer;

	info.clearValueCount = 1;
	info.pClearValues = nullptr;

	return info;
}

VkSemaphoreCreateInfo vkinit::semaphoreCreateInfo(VkSemaphoreCreateFlags flags /*= 0*/) {
	VkSemaphoreCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.pNext = nullptr;

    info.flags = flags;
	return info;
}

VkFenceCreateInfo vkinit::fenceCreateInfo(VkFenceCreateFlags flags /*= 0*/) {
	VkFenceCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.pNext = nullptr;

    // fence with signal on GPU command (for the first frame)
    info.flags = flags;
	return info;
}

VkSubmitInfo vkinit::submitInfo(const VkCommandBuffer* cmd) {
	VkSubmitInfo info {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.pNext = nullptr;

	info.waitSemaphoreCount = 0;
	info.pWaitSemaphores = nullptr;
	info.pWaitDstStageMask = nullptr;
	info.commandBufferCount = 1;
	info.pCommandBuffers = cmd;
	info.signalSemaphoreCount = 0;
	info.pSignalSemaphores = nullptr;

	return info;
}

VkPresentInfoKHR vkinit::presentInfo() {
	VkPresentInfoKHR info {};
	info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.pNext = nullptr;

	info.swapchainCount = 0;
	info.pSwapchains = nullptr;
	info.pWaitSemaphores = nullptr;
	info.waitSemaphoreCount = 0;
	info.pImageIndices = nullptr;
	info.pResults = nullptr; // Optional

	return info;
}