#include "MApplication.h"

#include "vk_initializers.h"

#include <stdexcept>
#include <array>
#include <cassert>
#include <iostream>

using namespace moo;

MApplication::MApplication() {
    loadModels();

    createPipelineLayout();
    reCreateSwapchain(); //createPipeline();
    createCommandBuffers();
}

MApplication::~MApplication() {
    vkDestroyPipelineLayout(m_device.getDevice(), m_pipelineLayout, nullptr);
}

void MApplication::run() {
    SDL_Event e;

    while (!m_window.isClosing())
    {
        while (SDL_PollEvent(&e) != 0)
        {
            m_window.handleEvent(e);
        }
        
        if(!m_window.isMinimized()) 
        {
            drawFrame();
        }
    }

    vkDeviceWaitIdle(m_device.getDevice());
}

void MApplication::createPipelineLayout() {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo();

    if(vkCreatePipelineLayout(m_device.getDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout.");
    }
}

void MApplication::createPipeline() {
    assert(m_swapchain != nullptr && "Cannot create pipeline before swapchain.");
    assert(m_pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout.");

    VulkanPipeline::PipelineConfigInfo pipelineConfig {};
    VulkanPipeline::defaultPipelineConfigInfo(pipelineConfig);
    
    pipelineConfig.renderpass = m_swapchain->getRenderPass();
    pipelineConfig.pipelineLayout = m_pipelineLayout;
    
    m_pipeline = std::make_unique<VulkanPipeline>(m_device, "./../shaders/shader.vert.spv", "./../shaders/shader.frag.spv", pipelineConfig);
}

void MApplication::reCreateSwapchain() {
    VkExtent2D extent = m_window.getExtent();

    vkDeviceWaitIdle(m_device.getDevice());

    if(m_swapchain == nullptr) {
        m_swapchain = std::make_unique<VulkanSwapchain>(m_device, extent);
    } else {
        m_swapchain = std::make_unique<VulkanSwapchain>(m_device, extent, std::move(m_swapchain));
        
        if (m_swapchain->imageCount() != m_commandBuffers.size()) {
            freeCommandBuffers();
            createCommandBuffers();
        } 
    }

    createPipeline();

    std::cout << "width: " << m_window.getExtent().width << "; height: " << m_window.getExtent().height << "\n";
}

void MApplication::createCommandBuffers() {
    m_commandBuffers.resize(m_swapchain->imageCount());

    VkCommandBufferAllocateInfo commandBufferInfo = vkinit::commandBufferAllocateInfo(m_device.getCommandPool(), static_cast<uint32_t>(m_commandBuffers.size()));

    if(vkAllocateCommandBuffers(m_device.getDevice(), &commandBufferInfo, m_commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers.");
    }    
}

void MApplication::freeCommandBuffers() {
    vkFreeCommandBuffers(m_device.getDevice(), m_device.getCommandPool(), static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
    m_commandBuffers.clear();
}

void MApplication::recordCommandBuffer(int imgIndex) {
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::commandBufferBeginInfo();

    if(vkBeginCommandBuffer(m_commandBuffers[imgIndex], &cmdBeginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer.");
    }

    VkRenderPassBeginInfo renderPassInfo = vkinit::renderpassBeginInfo(m_swapchain->getRenderPass(), m_swapchain->getSwapChainExtent(), m_swapchain->getFrameBuffer(imgIndex));
/*
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};
*/
    VkClearValue clearValues = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValues;

    vkCmdBeginRenderPass(m_commandBuffers[imgIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

// dynamic viewport scissor
    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapchain->getSwapChainExtent().width);
    viewport.height = static_cast<float>(m_swapchain->getSwapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor {};
    scissor.offset = {0, 0};
    scissor.extent = m_swapchain->getSwapChainExtent();

    vkCmdSetViewport(m_commandBuffers[imgIndex], 0, 1, &viewport);
    vkCmdSetScissor(m_commandBuffers[imgIndex], 0, 1, &scissor);

    // once we start adding rendering commands, 
    // CODE HERE |
    //           V

    m_pipeline->bind(m_commandBuffers[imgIndex]);
    model1->bind(m_commandBuffers[imgIndex]);
    model1->draw(m_commandBuffers[imgIndex]);
   
    //           ^
    // STOP HERE |

    vkCmdEndRenderPass(m_commandBuffers[imgIndex]);
    if(vkEndCommandBuffer(m_commandBuffers[imgIndex]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer.");
    }
}

void MApplication::drawFrame() {
    uint32_t imageIndex;
    VkResult result = m_swapchain->acquireNextImage(&imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        reCreateSwapchain();
        std::cout << "VK_ERROR_OUT_OF_DATE_KHR\n";
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swapchain image.");
    }

    recordCommandBuffer(imageIndex);
    result = m_swapchain->submitCommandBuffers(&m_commandBuffers[imageIndex], &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_window.wasWindowResized()) {
        m_window.resetWindowResizedFlag();
        reCreateSwapchain();
        std::cout << "m_window.wasWindowResized()\n";
        return;
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swapchain image.");
    }
}

void MApplication::loadModels() {
    VulkanModel::Builder builder{};

    builder.vertices =
    {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}}, 
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}} 
    };

    builder.indices =
    {0, 1, 2, 2, 3, 0};

    model1 = std::make_unique<VulkanModel>(m_device, builder);
}