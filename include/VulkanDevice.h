#pragma once

#include "VulkanDebug.h"

#include <vector>
#include <optional>

namespace moo {

class MWindow;

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> transfertFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value() && transfertFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanDevice {
public:
    VkPhysicalDeviceProperties m_deviceProperties;
    VkPhysicalDeviceFeatures m_deviceFeatures;
    VkPhysicalDeviceMemoryProperties m_deviceMemoryProperties;

private:
    MWindow& m_window;
    VkInstance m_instance;
    VulkanDebug m_debug;

    VkSurfaceKHR m_surface;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;

    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;
    VkQueue m_transfertQueue;

    VkCommandPool m_commandPool;

public:
    VulkanDevice(MWindow &window);
    ~VulkanDevice();

    // Not copyable or movable
    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete;
    VulkanDevice(VulkanDevice&&) = delete;
    VulkanDevice& operator=(VulkanDevice&&) = delete;

    inline VkDevice getDevice() { return m_device; }
    inline VkQueue getGraphicsQueue() { return m_graphicsQueue; }
    inline VkQueue getPresentQueue() { return m_presentQueue; }
    inline VkQueue getTransfertQueue() { return m_transfertQueue; }
    inline SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(m_physicalDevice); }
    inline QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(m_physicalDevice); }

    inline VkSurfaceKHR getSurface() { return m_surface; }
    inline VkCommandPool getCommandPool() { return m_commandPool; }

// buffer
    void createBuffer(
        VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
        VkBuffer &buffer, VkDeviceMemory &bufferMemory
    );

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    
/*
  
  VkFormat findSupportedFormat(
      const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

  // Buffer Helper Functions

  
  void copyBufferToImage(
      VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

  void createImageWithInfo(
      const VkImageCreateInfo &imageInfo,
      VkMemoryPropertyFlags properties,
      VkImage &image,
      VkDeviceMemory &imageMemory);

*/
private:
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createCommandPool();

// helper functions
// instance start
    std::vector<const char *> m_instanceExtensions;
    const std::vector<const char *> m_validationLayers = {"VK_LAYER_KHRONOS_validation"};
    const std::vector<const char *> m_deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    bool checkValidationLayerSupport(std::vector<const char *> validationLayers);
    // will return the required list of extensions based on which validation layers are enabled or not
    std::vector<const char *> getRequiredExtensions();
    bool checkExtensionSupport(std::vector<const char *> extensionList);
// instance end

// device start
    bool isDeviceSuitable(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    bool checkDeviceExtensionSupport(VkPhysicalDevice device, std::vector<const char*> deviceExtensions);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
// device end

// buffer start
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
// buffer end
};

}   // namespace moo