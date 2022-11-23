#pragma once

#include "MWindow.h"
#include "VulkanDebug.h"
#include "VulkanMesh.h"

#include <functional>
#include <deque>
#include <array>
#include <optional>

namespace mii {

constexpr unsigned int MAX_FRAMES_IN_FLIGHT = 2; // number of frames to overlap when rendering
constexpr int WIDTH = 640;
constexpr int HEIGHT = 360;

struct DeletionQueue {
	std::deque<std::function<void()>> deletors;

	inline void push_function(std::function<void()>&& function) {
		deletors.push_back(function);
	}

	inline void flush() {
		// reverse iterate the deletion queue to execute all the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)(); //call the function
		}

		deletors.clear();
	}
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
	std::optional<uint32_t> transfertFamily;

    inline bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value() && transfertFamily.has_value();
    }
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

// rendering related structures
struct FrameData {
	VkCommandPool m_commandPool;
	VkCommandBuffer m_mainCommandBuffer;

	VkSemaphore m_imageAvailableSemaphore;
	VkSemaphore m_renderFinishedSemaphore;
	VkFence m_inFlightFence;

    //AllocatedBuffer cameraBuffer; // buffer holds single gpu camera data
    //VkDescriptorSet globalDescriptor;

    //AllocatedBuffer objectBuffer;
    //VkDescriptorSet objectDescriptor;  
};

struct UploadContext {
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;
	VkFence transfer_fence;
};

class VulkanEngine {
public:
	VulkanEngine();
	~VulkanEngine();

	VulkanEngine(const VulkanEngine&) = delete;
	VulkanEngine& operator=(const VulkanEngine&) = delete;

	//initializes everything in the engine
	void init();

	//run main loop
	void run();

	//shuts down the engine
	void cleanup();
	
private:
	std::vector<const char*> m_instanceExtensions;

	const std::vector<const char*> m_validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> m_deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	uint32_t m_frameNumber {0};
	bool m_initialized = false;

	moo::MWindow m_window {"Amazing Mopugno", WIDTH, HEIGHT};

	VkInstance m_instance;
	moo::VulkanDebug m_debugger;

	VkPhysicalDevice m_gpu;
	VkDevice m_device;
	VkSurfaceKHR m_surface;

	VkPhysicalDeviceProperties m_deviceProperties;
	VkPhysicalDeviceFeatures m_deviceFeatures;
	VkPhysicalDeviceMemoryProperties m_deviceMemoryProperties;

	VkQueue	m_graphicsQueue;
	VkQueue	m_presentQueue;
	VkQueue m_transfertQueue;

	VkSwapchainKHR m_swapchain {nullptr};
	VkSwapchainKHR m_oldswapchain {nullptr};

	std::vector<VkImage> m_swapchainImages; // array of imgs from the swapchain
	VkFormat m_swapchainImageFormat; // img format expected by the window system
	VkExtent2D m_swapchainExtent;
	std::vector<VkImageView> m_swapchainImageViews; // vkimageview is a wrapper of vkimage
	
	std::vector<VkFramebuffer> m_swapchainFramebuffers;

	VkRenderPass m_renderPass;
	VkPipelineLayout m_defaultPipeLayout;
	VkPipeline m_defaultGraphicsPipeline;

	std::array<FrameData, MAX_FRAMES_IN_FLIGHT> m_frames;
	inline FrameData& get_current_frame() { return m_frames[m_frameNumber % MAX_FRAMES_IN_FLIGHT]; }

	DeletionQueue m_mainDeletionQueue;
	VmaAllocator m_allocator;

	Mesh triangle0 {}, triangle1 {};
	UploadContext m_uploadContext;

	void loadMeshes();
	void uploadMesh(Mesh &mesh); // create vertex buffer
	void createVertexBuffer(Mesh &mesh);
	void createIndexBuffer(Mesh &mesh);
	void createVertexIndexBuffer(Mesh &mesh);
	void createVertexIndexBufferT(Mesh &mesh);
	size_t pad_uniform_buffer_size(size_t originalSize);
	
	//uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties); // vma doing the work
	AllocatedBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags);
	void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size, VkDeviceSize srcOffeset, VkDeviceSize dstOffset);

	//draw loop
	void drawFrame();

	void createInstance();
	void setupDebugMessenger();
	void pickPhysicalDevice(VkInstance& instance);
	void createLogicalDevice(VkPhysicalDevice& physicalDevice);
	
	void cleanupSwapChain();
	void reCreateSwapchain();
	void createSwapchain();
	void createImageViews();
	void init_default_renderpass();
	void init_pipelines();
	void init_framebuffers();

	void createCommandPool();
	void createCommandBuffer();
	void init_sync_structures();

	inline void freeImageViews() {
		for (auto imageView : m_swapchainImageViews)
		{
			vkDestroyImageView(m_device, imageView, nullptr);
		}
	}

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	VkShaderModule loadShaderModuleFromFile(const std::string& filename);

// instance extensions
	bool checkExtensionSupport(std::vector<const char*> extensionList);
	// will return the required list of extensions based on which validation layers are enabled or not
	std::vector<const char*> requiredInstanceExtensions();

// instance layers
	bool checkValidationLayerSupport(std::vector<const char*> validationLayers);

// device start //
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR& surface);
    bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR& surface);

	// device extensions
	bool checkDeviceExtensionSupport(VkPhysicalDevice device, std::vector<const char*> deviceExtensions);
// device end //

// swapchain start //
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device); // checking for swapchain support

	// searching optimal setting for swapchain 
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
// swapchain end //	
};

}	// namespace mii