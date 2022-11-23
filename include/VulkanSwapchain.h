#pragma once

#include "VulkanDevice.h"

#include <memory>

namespace moo {

class VulkanSwapchain {
public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    VulkanSwapchain(VulkanDevice &deviceRef, VkExtent2D windowExtent);
    VulkanSwapchain(VulkanDevice &deviceRef, VkExtent2D windowExtent, std::shared_ptr<VulkanSwapchain> previous);
    ~VulkanSwapchain();

    VulkanSwapchain(const VulkanSwapchain&) = delete;
    VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

    VkFramebuffer getFrameBuffer(int index) { return m_swapChainFramebuffers[index]; }
    VkRenderPass getRenderPass() { return m_renderpass; }
    VkImageView getImageView(int index) { return m_swapChainImageViews[index]; }
    size_t imageCount() { return m_swapChainImages.size(); }
    VkFormat getSwapChainImageFormat() { return m_swapChainImageFormat; }
    VkExtent2D getSwapChainExtent() { return m_swapChainExtent; }
    uint32_t getWidth() { return m_swapChainExtent.width; }
    uint32_t getHeight() { return m_swapChainExtent.height; }

    float extentAspectRatio() {
        return static_cast<float>(m_swapChainExtent.width) / static_cast<float>(m_swapChainExtent.height);
    }
    VkFormat findDepthFormat();

    VkResult acquireNextImage(uint32_t *imageIndex);
    VkResult submitCommandBuffers(const VkCommandBuffer *buffers, uint32_t *imageIndex);

private:
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;

    std::vector<VkFramebuffer> m_swapChainFramebuffers;
    VkRenderPass m_renderpass;

    std::vector<VkImage> depthImages;
    std::vector<VkDeviceMemory> depthImageMemorys;
    std::vector<VkImageView> depthImageViews;
    std::vector<VkImage> m_swapChainImages;
    std::vector<VkImageView> m_swapChainImageViews;

    VulkanDevice &device;
    VkExtent2D m_windowExtent;

    VkSwapchainKHR m_swapchain;
    std::shared_ptr<VulkanSwapchain> m_oldSwapchain;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;

    void init();
    void createSwapChain();
    void createImageViews();
    void createDepthResources();
    void createRenderPass();
    void createFramebuffers();
    void createSyncObjects();

// Helper functions
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
};

}   // namespace moo