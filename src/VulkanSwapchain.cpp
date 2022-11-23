#include "VulkanSwapchain.h"

#include "vk_initializers.h"

// std
#include <array>
//#include <cstdlib>
//#include <cstring>
#include <iostream>
//#include <limits>
//#include <set>
//#include <stdexcept>

using namespace moo;

VulkanSwapchain::VulkanSwapchain(VulkanDevice &deviceRef, VkExtent2D extent) : device{deviceRef}, m_windowExtent{extent} {
    init();
}

VulkanSwapchain::VulkanSwapchain(VulkanDevice &deviceRef, VkExtent2D extent, std::shared_ptr<VulkanSwapchain> previous) 
: device{deviceRef}, m_windowExtent{extent}, m_oldSwapchain{previous} {
    init();

    // cleanup old swapchain since no longer needed
    m_oldSwapchain = nullptr;
}

void VulkanSwapchain::init() {
    createSwapChain();
    createImageViews();
    createRenderPass();
    //createDepthResources();
    createFramebuffers();
    createSyncObjects();
}

VulkanSwapchain::~VulkanSwapchain() {
    for (auto imageView : m_swapChainImageViews) {
        vkDestroyImageView(device.getDevice(), imageView, nullptr);
    }
    m_swapChainImageViews.clear();

    if (m_swapchain != nullptr) {
        vkDestroySwapchainKHR(device.getDevice(), m_swapchain, nullptr);
        m_swapchain = nullptr;
    }

    for (int i = 0; i < depthImages.size(); i++) {
        vkDestroyImageView(device.getDevice(), depthImageViews[i], nullptr);
        vkDestroyImage(device.getDevice(), depthImages[i], nullptr);
        vkFreeMemory(device.getDevice(), depthImageMemorys[i], nullptr);
    }

    for (auto framebuffer : m_swapChainFramebuffers) {
        vkDestroyFramebuffer(device.getDevice(), framebuffer, nullptr);
    }

    vkDestroyRenderPass(device.getDevice(), m_renderpass, nullptr);

    // cleanup synchronization objects
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device.getDevice(), renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device.getDevice(), imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device.getDevice(), inFlightFences[i], nullptr);
    }
}

VkResult VulkanSwapchain::acquireNextImage(uint32_t *imageIndex) {
    vkWaitForFences(device.getDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

    VkResult result = vkAcquireNextImageKHR(device.getDevice(), m_swapchain, std::numeric_limits<uint64_t>::max(),
        imageAvailableSemaphores[currentFrame],  // must be a not signaled semaphore
        VK_NULL_HANDLE, imageIndex);

    return result;
}

VkResult VulkanSwapchain::submitCommandBuffers(const VkCommandBuffer *buffers, uint32_t *imageIndex) {
    if(imagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device.getDevice(), 1, &imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
    }
    imagesInFlight[*imageIndex] = inFlightFences[currentFrame];

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    std::array<VkSemaphore, 1> waitSemaphores = {imageAvailableSemaphores[currentFrame]};
    std::array<VkPipelineStageFlags, 1> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitStages.data();

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = buffers;

    std::array<VkSemaphore, 1> signalSemaphores = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores.data();

    vkResetFences(device.getDevice(), 1, &inFlightFences[currentFrame]);
    if (vkQueueSubmit(device.getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer.");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores.data();

    std::array<VkSwapchainKHR, 1> swapChains = {m_swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains.data();

    presentInfo.pImageIndices = imageIndex;

    VkResult result = vkQueuePresentKHR(device.getPresentQueue(), &presentInfo);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    return result;
}

void VulkanSwapchain::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = device.getSwapChainSupport();

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = device.getSurface();

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = device.findPhysicalQueueFamilies();
    //TODO(): check indices between queue families properly, currently is assuming all having same index
    std::array<uint32_t, 3> queueFamilyIndices = {indices.graphicsFamily.value(), indices.presentFamily.value(), indices.transfertFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 3;
        createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;      // Optional
        createInfo.pQueueFamilyIndices = nullptr;  // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = m_oldSwapchain == nullptr ? VK_NULL_HANDLE : m_oldSwapchain->m_swapchain;

    if (vkCreateSwapchainKHR(device.getDevice(), &createInfo, nullptr, &m_swapchain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swapchain.");
    }

    // we only specified a minimum number of images in the swap chain, so the implementation is
    // allowed to create a swap chain with more. That's why we'll first query the final number of
    // images with vkGetSwapchainImagesKHR, then resize the container and finally call it again to
    // retrieve the handles.
    vkGetSwapchainImagesKHR(device.getDevice(), m_swapchain, &imageCount, nullptr);
    m_swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device.getDevice(), m_swapchain, &imageCount, m_swapChainImages.data());

    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;
}

void VulkanSwapchain::createImageViews() {
    size_t imageCount = m_swapChainImages.size();
    m_swapChainImageViews.resize(imageCount);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_swapChainImageFormat;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    for (size_t i = 0; i < imageCount; i++) {    
        viewInfo.image = m_swapChainImages[i];

        if (vkCreateImageView(device.getDevice(), &viewInfo, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create texture image view.");
        }
    }
}

void VulkanSwapchain::createRenderPass() {
// ATTACHMENTs
    // define an attachment description for main color image
	// attachment is loaded as "clear" when renderpass start
	// attachment is stored when renderpass ends
	// attachment layout starts as "undefined", and transitions to "Present" so its possible to display it
	// dont care about stencil, and dont use multisampling

// 1st attachment
    VkAttachmentDescription colorAttachment {};
    colorAttachment.format = getSwapChainImageFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    // apply to color and depth data:
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear data values to a constant at the start
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // rendered contents will be stored in memory and can be read later

    // don't care about stencil
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // don't know or care about starting layout of the attachment because the content is going to be cleared anyway
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    // after renderpass ends the image has to be on the layout ready for display
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // subpass info
    VkAttachmentReference colorAttachmentRef {};
    colorAttachmentRef.attachment = 0;  // index of pAttachments array in the parent renderpass
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

/*
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
*/

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    //subpass.pDepthStencilAttachment = &depthAttachmentRef;

// subpass dependencies
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcAccessMask = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstSubpass = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
/*
    dependency.srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstSubpass = 0;
    dependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
*/
    //std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    std::array<VkSubpassDependency, 1> dependencies = {dependency};
    std::array<VkAttachmentDescription, 1> attachments = {colorAttachment};

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(device.getDevice(), &renderPassInfo, nullptr, &m_renderpass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create renderpass.");
    }
}

void VulkanSwapchain::createFramebuffers() {
    VkFramebufferCreateInfo framebufferInfo = vkinit::framebufferCreateInfo(getRenderPass(), getSwapChainExtent());

    m_swapChainFramebuffers.resize(imageCount());
    for (size_t i = 0; i < imageCount(); i++) {
        //std::array<VkImageView, 2> attachments = {m_swapChainImageViews[i], depthImageViews[i]};
        std::array<VkImageView, 1> attachments = {m_swapChainImageViews[i]};

        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();

        if (vkCreateFramebuffer(device.getDevice(), &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffers.");
        }
    }
}
/*
void VulkanSwapchain::createDepthResources() {
  VkFormat depthFormat = findDepthFormat();
  VkExtent2D swapChainExtent = getSwapChainExtent();

  depthImages.resize(imageCount());
  depthImageMemorys.resize(imageCount());
  depthImageViews.resize(imageCount());

  for (int i = 0; i < depthImages.size(); i++) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = swapChainExtent.width;
    imageInfo.extent.height = swapChainExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags = 0;

    device.createImageWithInfo(
        imageInfo,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        depthImages[i],
        depthImageMemorys[i]);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = depthImages[i];
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device.getDevice(), &viewInfo, nullptr, &depthImageViews[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create texture image view!");
    }
  }
}
*/
void VulkanSwapchain::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(imageCount(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo = vkinit::semaphoreCreateInfo(); // semaphores flags aren't needed
    VkFenceCreateInfo fenceInfo = vkinit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT); // want the fence to start signaled so we can wait on it on the first frame

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device.getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device.getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device.getDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create synchronization objects for a frame.");
        }
    }
}

VkSurfaceFormatKHR VulkanSwapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
    for (const auto &availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR VulkanSwapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
    for (const auto &availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            std::cout << "Present mode: Mailbox" << "\n";
            return availablePresentMode;
        }
    }

  // for (const auto &availablePresentMode : availablePresentModes) {
  //   if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
  //     std::cout << "Present mode: Immediate" << std::endl;
  //     return availablePresentMode;
  //   }
  // }

    std::cout << "Present mode: V-Sync" << "\n";
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = m_windowExtent;
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
    }
}
/*
VkFormat VulkanSwapchain::findDepthFormat() {
  return device.findSupportedFormat(
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL,
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}
*/
