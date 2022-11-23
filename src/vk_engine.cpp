#include "vk_engine.h"

#include <SDL_vulkan.h>
#include "vk_initializers.h"
#include "vk_pipeline.h"

#include <iostream>
#include <fstream>
#include <set>
#include <cassert>

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

#define VK_CHECK(x)                                         \
    do                                                      \
    {                                                       \
        VkResult err = x;                                   \
        if(err)                                             \
        {                                                   \
		    std::string error = "Detected Vulkan error: " + std::to_string(err); \
            throw std::runtime_error(error);                \
        }                                                   \
    } while(0)                                              \

using namespace mii;

VulkanEngine::VulkanEngine() {}
VulkanEngine::~VulkanEngine() {}

bool VulkanEngine::isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR& surface) {
    QueueFamilyIndices indices = findQueueFamilies(device, surface);

    bool extensionsSupported = checkDeviceExtensionSupport(device, m_deviceExtensions);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

QueueFamilyIndices VulkanEngine::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR& surface) {
    // logic to find graphics queue family
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    uint32_t i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) {
                indices.transfertFamily = i;
            }
        }
        
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (presentSupport && !indices.presentFamily.has_value()) {
            indices.presentFamily = i;
        }

        if(indices.isComplete())
            break;

        i++;
    }

    return indices;
}

std::vector<const char*> VulkanEngine::requiredInstanceExtensions() {
	unsigned int extensionCount;
	if(!SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, nullptr)) { 
        std::string error = SDL_GetError();			
	    throw std::runtime_error("Detected SDL error: " + error);     
    }

	std::vector<const char*> extensionList(extensionCount);
	if(!SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, extensionList.data())) { 
        std::string error = SDL_GetError();			
        throw std::runtime_error("Detected SDL error: " + error);     
    }

	if(enableValidationLayers) {
		extensionList.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensionList;
}

bool VulkanEngine::checkValidationLayerSupport(std::vector<const char*> validationLayers) {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	// check if all layers in validationLayers exist in avaibleLayers list
	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if(strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if(!layerFound) {
			return false;
		}
	}

	return true;
}

bool VulkanEngine::checkExtensionSupport(std::vector<const char*> extensionList) {
	uint32_t extensionCount;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

	for (const char* extensionName : extensionList)
	{
		bool extensionFound = false;

		for(const auto& extensionProperties : availableExtensions) {
			if(strcmp(extensionName, extensionProperties.extensionName) == 0) {
				extensionFound = true;
				break;
			}
		}

		if(!extensionFound) {
			return false;
		}
	}

	return true;
}

bool VulkanEngine::checkDeviceExtensionSupport(VkPhysicalDevice device, std::vector<const char*> deviceExtensions) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

VkShaderModule VulkanEngine::loadShaderModuleFromFile(const std::string& filename) {
    // open file, cursor at the end
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if(!file.is_open())
        throw std::runtime_error("Failed to open file: " + filename);

    std::streampos filesize = file.tellg();
    //size_t filesize = (size_t)file.tellg();

    // spirv expects the buffer to be an uint32 instead of a char,
    // reserve an int vector big enough for the entire file ELEMENTS
    //std::vector<uint32_t> buffer(filesize / sizeof(uint32_t)); // bytes -> uint32
    // std::vector<char> buffer(fileSize); equals to 
    std::vector<char> buffer(filesize / sizeof(char));
    file.seekg(0, std::ios::beg); // file cursor at the beginning, rdy to read 
    file.read(buffer.data(), filesize);
    //file.read((char*)buffer.data(), filesize);
    file.close();

    VkShaderModuleCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = buffer.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

    // check creation went well, 
    // cause there are often error VK_CHECK isn't suitable
    VkShaderModule shaderModule;
    if(vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule)
        != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module for: " + filename);
    }
    return shaderModule;
}

SwapChainSupportDetails VulkanEngine::querySwapChainSupport(VkPhysicalDevice device) {
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);
	
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
	}

    return details;
}

VkSurfaceFormatKHR VulkanEngine::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR VulkanEngine::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanEngine::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = m_window.getExtent();
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
    }
}

void VulkanEngine::init() {
    // instance
    createInstance();

    // debugging
    setupDebugMessenger();
    
	// surface
    m_window.createWindowSurface(m_instance, &m_surface);

    // physical device
    pickPhysicalDevice(m_instance);

	// logical device
    createLogicalDevice(m_gpu);

    // swapchain
    createSwapchain();
    createImageViews();
	init_default_renderpass();
	init_pipelines();
    init_framebuffers();

    createCommandPool();
    createCommandBuffer();
    init_sync_structures();   

    loadMeshes();
    //createVertexBuffer(); // delete

    m_initialized = true;
}

void VulkanEngine::cleanup() {	
	if (m_initialized) {
        // make sure GPU has stopped doing its things: 
        // drawing and presentation operations may still be going on. 
        // Cleaning up resources while that is happening will crush app.
        vkDeviceWaitIdle(m_device);

        m_mainDeletionQueue.flush(); // commandpool and semaphore/fences

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroyCommandPool(m_device, m_frames[i].m_commandPool, nullptr);
        }
        
        cleanupSwapChain();

        vmaDestroyAllocator(m_allocator);
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        vkDestroyDevice(m_device, nullptr);
        
        if(enableValidationLayers)
		    m_debugger.freeDebugCallback(m_instance);
		
        vkDestroyInstance(m_instance, nullptr);
    }
}

void VulkanEngine::cleanupSwapChain() {
    for (auto framebuffers : m_swapchainFramebuffers)
    {
        vkDestroyFramebuffer(m_device, framebuffers, nullptr);
    }

    vkDestroyPipeline(m_device, m_defaultGraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_defaultPipeLayout, nullptr);
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);

    freeImageViews();

    // detroy swapchain + surface + iamge view
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
}

void VulkanEngine::reCreateSwapchain() {
    vkDeviceWaitIdle(m_device);

    assert(m_swapchain != nullptr);

    for (auto framebuffers : m_swapchainFramebuffers)
    {
        vkDestroyFramebuffer(m_device, framebuffers, nullptr);
    }

    vkDestroyPipeline(m_device, m_defaultGraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_defaultPipeLayout, nullptr);
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);

    freeImageViews();

    // detroy swapchain + surface + iamge view
    //vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    	
    createSwapchain();
    createImageViews();
    init_default_renderpass();
    init_pipelines();
    init_framebuffers();

    /*if (m_swapchainImages.size() != m_frames.size()) {
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroyCommandPool(m_device, m_frames[i].m_commandPool, nullptr);
        }
        createCommandPool();
        createCommandBuffer();
    }*/ 

    std::cout << "width: " << m_window.getExtent().width << "; height: " << m_window.getExtent().height << "\n";
}

void VulkanEngine::drawFrame() {
// 1st: waiting for the previous frame
    // CPU waits until GPU has finished rendering last frame, timeout 1 sec
    VK_CHECK(vkWaitForFences(m_device, 1, &get_current_frame().m_inFlightFence, VK_TRUE, 1000000000ULL)); // 1 sec in nanoseconds 10^9; UINT64_MAX disables the timeout

// 2nd: acquiring an image from the swap chain
    // request img from swapchain, 1 sec timeout
    uint32_t swapchain_imageIndex;
    // obj to be signaled when presentation engine finished using the image
    VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, 1000000000ULL, get_current_frame().m_imageAvailableSemaphore, nullptr, &swapchain_imageIndex); 
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        reCreateSwapchain();
        std::cout << "First check\n";
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swapchain image");
    }

    // reset after acquiring image to avoid deadlock caused by the return in the case out of date error (ex. resized window)
    // only reset the fence if we are submitting work
    VK_CHECK(vkResetFences(m_device, 1, &get_current_frame().m_inFlightFence));

// 3rd: recording the command buffer
    // now that it's sure commands finished executing, safely reset command buffer to beging recording again
    VK_CHECK(vkResetCommandBuffer(get_current_frame().m_mainCommandBuffer, 0));

    VkCommandBuffer &cmd = get_current_frame().m_mainCommandBuffer; // alias shorter writing

    recordCommandBuffer(cmd, swapchain_imageIndex);

// 4th: submitting the command buffer
    // prepare the submission to the queue GPU
    // wait on the _presentSemaphore, it's signaled when swapchain is ready
    // signal on the _renderSemaphore, to signal rendering has finished
    VkSubmitInfo submit = vkinit::submitInfo(&cmd);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    std::array<VkSemaphore, 1> waitSemaphores = {get_current_frame().m_imageAvailableSemaphore};
    std::array<VkSemaphore, 1> signalSemaphores = {get_current_frame().m_renderFinishedSemaphore};

    submit.pWaitDstStageMask = &waitStage;
    submit.waitSemaphoreCount = 1;
    //submit.pWaitSemaphores = &get_current_frame()._imageAvailableSemaphore;
    submit.pWaitSemaphores = waitSemaphores.data();

    submit.signalSemaphoreCount = 1;
    //submit.pSignalSemaphores = &get_current_frame()._renderFinishedSemaphore;
    submit.pSignalSemaphores = signalSemaphores.data();

    // submit command buffer to the queue and execute it
    // m_inFlightFence will block until graphic commands finish execution
    // last parameter references an optional fence that will be signaled when the command buffers finish execution
    // the CPU will wait for this command buffer to finish executing before it records new commands into it
    VK_CHECK(vkQueueSubmit(m_graphicsQueue, 1, &submit, get_current_frame().m_inFlightFence));

// presentation
    // prepare present
    // the image just rendered will be put into the visible window
    // wait on the _renderSemaphore for that
    // cause it's necessary drawing commands being finished before the img is displayed to the user
    VkPresentInfoKHR present_info = vkinit::presentInfo();

    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signalSemaphores.data();
    
    std::array<VkSwapchainKHR, 1> swapchains = {m_swapchain};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains.data();
    present_info.pImageIndices = &swapchain_imageIndex;
    
    result = vkQueuePresentKHR(m_graphicsQueue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_window.wasWindowResized()) {
        m_window.resetWindowResizedFlag();
        reCreateSwapchain();
        //return;
        std::cout << "Second check\n";
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swapchain image.");
    }

    m_frameNumber = (m_frameNumber + 1) % MAX_FRAMES_IN_FLIGHT; // increase number of frames drawn
}

void VulkanEngine::run() {
	SDL_Event e;

	//main loop
	while (!m_window.isClosing())
	{
		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
            m_window.handleEvent(e);            
		}
        
        if(!m_window.isMinimized()) {
		    drawFrame();
        }
	}
}

void VulkanEngine::createInstance() {
    // EXTENSIONS
    m_instanceExtensions = requiredInstanceExtensions();
	std::vector<const char*> instance_extensions = m_instanceExtensions;

	if(enableValidationLayers) { 
        if(!checkExtensionSupport(instance_extensions)) {
		    throw std::runtime_error("Extensions requested, but not available.");
	    } else {
            std::cout << "Enabled extensions:\n";
            for (const auto& exts : instance_extensions) {
                std::cout << '\t' << exts << '\n';
            }
        }
    }

    //LAYERS
    std::vector<const char*> instance_validationLayers = m_validationLayers;
	if(enableValidationLayers) {
        if(!checkValidationLayerSupport(instance_validationLayers)) {
		    throw std::runtime_error("Validation layers requested, but not available.");
        } else {
            std::cout << "Enabled validation layers:\n";
            for (const auto& lys : instance_validationLayers) {
                std::cout << '\t' << lys << '\n';
            }
        }
	}

    VkApplicationInfo app_info {};    
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Mopugno";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Vulkan";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

	// struct is not optional and tells the Vulkan driver which global extensions and validation layers we want to use
	//  Global here means that they apply to the entire program and not a specific device
	VkInstanceCreateInfo instance_cInfo {};
	instance_cInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_cInfo.pApplicationInfo = &app_info;
	instance_cInfo.enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size());
	instance_cInfo.ppEnabledExtensionNames = instance_extensions.data();

	if(enableValidationLayers) {
		instance_cInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
		instance_cInfo.ppEnabledLayerNames = m_validationLayers.data();

        // create a debug utils messenger will trigger debugUtilsMessengerCallback
        // to cover instance creation/destro
        VkDebugUtilsMessengerCreateInfoEXT debugMessenger_cInfo {};
        debugMessenger_cInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugMessenger_cInfo.messageSeverity = 
            //VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            //VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugMessenger_cInfo.messageType = 
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugMessenger_cInfo.pfnUserCallback = m_debugger.debugUtilsMessengerCallback;

		instance_cInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugMessenger_cInfo;
	} else {
		instance_cInfo.enabledLayerCount = 0;

		instance_cInfo.pNext = nullptr;
	}
	
	VK_CHECK(vkCreateInstance(&instance_cInfo, nullptr, &m_instance));
}

void VulkanEngine::setupDebugMessenger() {
    if(enableValidationLayers)
        m_debugger.setupDebugging(m_instance);
}

void VulkanEngine::pickPhysicalDevice(VkInstance& instance) {
    // find physical devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    
    if (deviceCount == 0) 
        throw std::runtime_error("Failed to find GPUs with Vulkan support.");

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

    // gpu selection
    for(const VkPhysicalDevice& device : physicalDevices) {
        if(isDeviceSuitable(device, m_surface)) {
            m_gpu = device;
            break;
        }
    }

    if(m_gpu == VK_NULL_HANDLE)
        throw std::runtime_error("Failed to find a suitable GPU.");
    
    // store data
    vkGetPhysicalDeviceProperties(m_gpu, &m_deviceProperties);
	vkGetPhysicalDeviceFeatures(m_gpu, &m_deviceFeatures);
	vkGetPhysicalDeviceMemoryProperties(m_gpu, &m_deviceMemoryProperties);

    std::cout << "The GPU has a minimum buffer alignment of " << m_deviceProperties.limits.minUniformBufferOffsetAlignment << "\n";
}

void VulkanEngine::createLogicalDevice(VkPhysicalDevice& gpu) {
    QueueFamilyIndices indices = findQueueFamilies(gpu, m_surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value(), indices.presentFamily.value(), indices.transfertFamily.value()
    };

    float queuePriority = 1.0f;

    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfos.push_back(queueCreateInfo);
    }

    // features
    VkPhysicalDeviceFeatures deviceFeatures {};

    VkDeviceCreateInfo deviceCreateInfo {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    // extensions
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

    // layers
    // enabledLayerCount and ppEnabledLayerNames fields of VkDeviceCreateInfo are ignored by up-to-date implementations. 
    // However, it is still a good idea to set them anyway to be compatible with older implementations
    if (enableValidationLayers) {
        deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
        deviceCreateInfo.ppEnabledLayerNames = m_validationLayers.data();
    } else {
        deviceCreateInfo.enabledLayerCount = 0;
    }

    VK_CHECK(vkCreateDevice(gpu, &deviceCreateInfo, nullptr, &m_device));

    // store data
    vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
    vkGetDeviceQueue(m_device, indices.transfertFamily.value(), 0, &m_transfertQueue);

    //vma allocator
    VmaAllocatorCreateInfo allocInfo {};
    allocInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    allocInfo.physicalDevice = m_gpu;
    allocInfo.device = m_device;
    allocInfo.instance = m_instance;
    vmaCreateAllocator(&allocInfo, &m_allocator);
}

void VulkanEngine::createSwapchain() {
	// Store the current swap chain handle so we can use it later on to ease up recreation
	m_oldswapchain = m_swapchain == nullptr ? nullptr : m_swapchain;

    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_gpu);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D swapchainExtent = chooseSwapExtent(swapChainSupport.capabilities);
	
    // how many images we would like to have in the swap chain
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = swapchainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(m_gpu, m_surface);
    
    if(indices.graphicsFamily != indices.presentFamily) {
        std::array<uint32_t, 3> queueFamilyIndices = {indices.graphicsFamily.value(), indices.presentFamily.value(), indices.transfertFamily.value()};
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 3;
        createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    } else {
        std::array<uint32_t, 2> queueFamilyIndices = {indices.graphicsFamily.value(), indices.transfertFamily.value()};
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // VK_SHARING_MODE_EXCLUSIVE
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    }
    
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // ignore alpha channel blending with other windows in the window system
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = m_oldswapchain; // for when swap chain becomes invalid or unoptimized, aka resizing
    
    VK_CHECK(vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain));

    // store swapchain's related images
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
    m_swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data());
    m_swapchainImageFormat = surfaceFormat.format;
    m_swapchainExtent = swapchainExtent;
}

void VulkanEngine::createImageViews() {
    size_t imageCount = m_swapchainImages.size();
    m_swapchainImageViews.resize(imageCount);

    for (size_t i = 0; i < imageCount; i++)
    {
        VkImageViewCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_swapchainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(m_device, &createInfo, nullptr, &m_swapchainImageViews[i]));
    }
}

void VulkanEngine::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_gpu, m_surface);
    uint32_t graphicsIndex = queueFamilyIndices.graphicsFamily.value();
    uint32_t transferIndex = queueFamilyIndices.transfertFamily.value();

    // create command pool for commands submitted to the graphics queue
    // the command pool will be one that can submit GRAPHICS commands
    // also want the pool to allow for resetting of individual command buffer
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::commandPoolCreateInfo(graphicsIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VK_CHECK(vkCreateCommandPool(m_device, &commandPoolInfo, nullptr, &m_frames[i].m_commandPool));
    }

    VkCommandPoolCreateInfo uploadCommandPoolInfo = vkinit::commandPoolCreateInfo(transferIndex, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
    // create pool for context
    VK_CHECK(vkCreateCommandPool(m_device, &uploadCommandPoolInfo, nullptr, &m_uploadContext.commandPool));
    m_mainDeletionQueue.push_function([=]() {
        vkDestroyCommandPool(m_device, m_uploadContext.commandPool, nullptr);
    });
}

void VulkanEngine::createCommandBuffer() {
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        // allocate default command buffer used for rendering
        // commands will be made from _command_pool
        // allocate 1 command buffer
        // command lvl is Primary
        VkCommandBufferAllocateInfo commandBufferInfo = vkinit::commandBufferAllocateInfo(m_frames[i].m_commandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(m_device, &commandBufferInfo, &m_frames[i].m_mainCommandBuffer));
    }

    VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::commandBufferAllocateInfo(m_uploadContext.commandPool, 1);
    VK_CHECK(vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &m_uploadContext.commandBuffer));
}

void VulkanEngine::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    // begin command buffer recording
    // using this command buffer once, let Vulkan knows about that
    //VkCommandBufferBeginInfo cmdBegingInfo = vkinit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::commandBufferBeginInfo();
    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo));

    VkRenderPassBeginInfo renderPassInfo = vkinit::renderpassBeginInfo(m_renderPass, m_swapchainExtent, m_swapchainFramebuffers[imageIndex]);
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

// dynamic viewport and scissors
    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapchainExtent.width);
    viewport.height = static_cast<float>(m_swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor {};
    scissor.offset = {0, 0};
    scissor.extent = m_swapchainExtent;

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // once we start adding rendering commands, 
    // CODE HERE |
    //           V

    //draw_objects(cmd, _renderables.data(), static_cast<uint32_t>(_renderables.size()));
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_defaultGraphicsPipeline);

    triangle0.bind(commandBuffer);
    triangle0.draw(commandBuffer);

    //           ^
    // STOP HERE |

    // finalize renderpass
    vkCmdEndRenderPass(commandBuffer);
    // finalize command buffer: can no longer add commands, but can be executed
    VK_CHECK(vkEndCommandBuffer(commandBuffer));
}

void VulkanEngine::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size, VkDeviceSize srcOffeset, VkDeviceSize dstOffset) {
    VkCommandBuffer cmd = m_uploadContext.commandBuffer;

    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = srcOffeset; // Optional
    copyRegion.dstOffset = dstOffset; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(cmd, src, dst, 1, &copyRegion);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    VK_CHECK(vkQueueSubmit(m_transfertQueue, 1, &submitInfo, m_uploadContext.transfer_fence));

    vkWaitForFences(m_device, 1, &m_uploadContext.transfer_fence, true, 9999999999);
    vkResetFences(m_device, 1, &m_uploadContext.transfer_fence);

    vkResetCommandPool(m_device, m_uploadContext.commandPool, 0);
}

AllocatedBuffer VulkanEngine::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags) {
    AllocatedBuffer newBuffer;

    VkBufferCreateInfo bufferInfo {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo {};
    allocInfo.usage = memoryUsage;
    allocInfo.flags = flags;
    

    vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &newBuffer.buffer, &newBuffer.allocation, nullptr);

    return newBuffer;
}

/*TO DELETE
uint32_t VulkanEngine::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_gpu, &memProperties);

    for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type.");
}*/

void VulkanEngine::init_sync_structures() {
    // create synch structures
    // one fence to control when the gpu has finished rendering the frame,
	// and 2 semaphores to syncronize rendering with swapchain
	
    VkFenceCreateInfo fenceCreateInfo = vkinit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT); // want the fence to start signaled so we can wait on it on the first frame
    VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphoreCreateInfo(); // semaphores flags aren't needed

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VK_CHECK(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_frames[i].m_inFlightFence));

        VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_frames[i].m_imageAvailableSemaphore));
        VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_frames[i].m_renderFinishedSemaphore));

            // enqueue destruction for fence
        m_mainDeletionQueue.push_function([=]() {
            vkDestroyFence(m_device, m_frames[i].m_inFlightFence, nullptr);

            vkDestroySemaphore(m_device, m_frames[i].m_imageAvailableSemaphore, nullptr);
            vkDestroySemaphore(m_device, m_frames[i].m_renderFinishedSemaphore, nullptr);
        });
    }

    VkFenceCreateInfo uploadFenceCreateInfo = vkinit::fenceCreateInfo();

    VK_CHECK(vkCreateFence(m_device, &uploadFenceCreateInfo, nullptr, &m_uploadContext.transfer_fence));
    m_mainDeletionQueue.push_function([=]() {
        vkDestroyFence(m_device, m_uploadContext.transfer_fence, nullptr);
    });
}

void VulkanEngine::init_default_renderpass() {
// ATTACHMENTs
    // define an attachment description for main color image
	// attachment is loaded as "clear" when renderpass start
	// attachment is stored when renderpass ends
	// attachment layout starts as "undefined", and transitions to "Present" so its possible to display it
	// dont care about stencil, and dont use multisampling

    // 1st attachment
    VkAttachmentDescription colorAttachment {}; // renderpass will use this color attachment
    colorAttachment.format = m_swapchainImageFormat; // attachment will have the format needed by the swapchain
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // 1 sample, won't be doing MSAA

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

    // SUBPASS to render into
    VkAttachmentReference colorAttachmentRef {};
    // attacment number will index into the pAttachments array in the parent renderpass itself
    colorAttachmentRef.attachment = 0; // 1 attachment: the above colotAttachment
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

/*
    // depth attachment
    VkAttachmentDescription depth_attachment {};
    depth_attachment.flags = 0;
    depth_attachment.format = _depthFormat;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref {};
    depth_attachment_ref.attachment = 1;
	depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
*/

    // going to create 1 subpass, minimum
    VkSubpassDescription subpass {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    // hook depth attachment into the subpass
    //subpass.pDepthStencilAttachment = &depth_attachment_ref;

// subpass dependencies
    // dependencies for synchronization 1 and 2
    // 1 dependency, which is from outside into the subpass
    // we can read or write color
    
	VkSubpassDependency color_dependency {};
	color_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	color_dependency.dstSubpass = 0;
	color_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	color_dependency.srcAccessMask = 0;
	color_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	color_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // dependency from outside to the subpass, making this subpass dependent on the previous renderpasses
/*  VkSubpassDependency depth_dependency {};
    depth_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    depth_dependency.dstSubpass = 0;
    depth_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depth_dependency.srcAccessMask = 0;
    depth_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depth_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
*/
/*
    // array of 2 dependencies:  for color; for depth
	//VkSubpassDependency dependencies[2] = {color_dependency, depth_dependency};

    // RENDERPASS
    // need to add the depth attachment to the attachment list in the renderpass itself

    // array of 2 attachments, one for color, other for depth
    //VkAttachmentDescription attachments[2] = { color_attachment, depth_attachment };
*/
    std::array<VkSubpassDependency, 1> dependencies = {color_dependency};
    std::array<VkAttachmentDescription, 1> attachments = {colorAttachment};

    VkRenderPassCreateInfo renderPassInfo {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pNext = nullptr;
    // 2 attachments from attachment array
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    // connect subpass to the info
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    // 2 dependencies from dependency array
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    VK_CHECK(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass));
}

void VulkanEngine::init_pipelines() {
// Shader stages
    VkShaderModule vertShaderModule = loadShaderModuleFromFile("./../shaders/shader.vert.spv");
    VkShaderModule fragShaderModule = loadShaderModuleFromFile("./../shaders/shader.frag.spv");

    PipelineBuilder pipelineBuilder;
// pipeline start
    pipelineBuilder.shaderStages.push_back(vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule));
    pipelineBuilder.shaderStages.push_back(vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule));
    // vertex input
    pipelineBuilder.vertexInputInfo = vkinit::pipelineVertexInputCreateInfo();
    // input assembly
    pipelineBuilder.inputAssembly = vkinit::pipeInputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    pipelineBuilder.viewportStateInfo = vkinit::pipelineViewportStateInfo();
    // rasterizer
    pipelineBuilder.rasterizer = vkinit::rasterizationStageCreateInfo(VK_POLYGON_MODE_FILL);
    // multisampling
    pipelineBuilder.multisampling = vkinit::multisamplingCreateInfo();
    // depth and stencil testing
    
    // color blending configuration per attached framebuffer
    pipelineBuilder.colorBlendAttachment = vkinit::colorblendAttachmentState();
    
    // dynamic state
    std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicStateInfo {};
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicStateInfo.pDynamicStates = dynamicStates.data();

    pipelineBuilder.dynamicStateInfo = dynamicStateInfo;

    // vertex input description
    VertexInputDescription vertexDescription = Vertex::getVertexInputDescription();
    pipelineBuilder.vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexDescription.bindings.size());
    pipelineBuilder.vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
    pipelineBuilder.vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexDescription.attributes.size());
    pipelineBuilder.vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
    
    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo();
        
    VK_CHECK(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_defaultPipeLayout));

    pipelineBuilder.pipelineLayout = m_defaultPipeLayout;
// pipeline end

// create pipeline
    m_defaultGraphicsPipeline = pipelineBuilder.build_pipeline(m_device, m_renderPass);

    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
}

void VulkanEngine::init_framebuffers() {
    // create framebuffers for the swapchain images.
    // Will connect renderpass to the images for rendering  
    VkFramebufferCreateInfo framebuffer_info = vkinit::framebufferCreateInfo(m_renderPass, m_swapchainExtent);
    
    // grab how many imgs have in the swapchain
    const size_t swapchain_imgcount = m_swapchainImageViews.size();
    m_swapchainFramebuffers.resize(swapchain_imgcount);

    // create framebuffers for each swapchain img views
    for(size_t i = 0; i < swapchain_imgcount; i++) {

        //VkImageView attachments[2];
        //attachments[0] = _swapchainImageViews[i];
        //attachments[1] = _depthImageView;

        std::array<VkImageView, 1> attachments = {m_swapchainImageViews[i]};

        framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size()); //2
		framebuffer_info.pAttachments = attachments.data();
        
        VK_CHECK(vkCreateFramebuffer(m_device, &framebuffer_info, nullptr, &m_swapchainFramebuffers[i]));
    }
}

void VulkanEngine::loadMeshes() {
    triangle0.vertices.resize(4);

    triangle0.vertices[0].position = {-0.5f, -0.5f, 0.0f};
    triangle0.vertices[1].position = {0.5f, -0.5f, 0.0f};
    triangle0.vertices[2].position = {0.5f, 0.5f, 0.0f};
    triangle0.vertices[3].position = {-0.5f, 0.5f, 0.0f};

    triangle0.vertices[0].color = {1.0f, 0.0f, 0.0f};
    triangle0.vertices[1].color = {0.0f, 1.0f, 0.0f};
    triangle0.vertices[2].color = {0.0f, 0.0f, 1.0f};
    triangle0.vertices[3].color = {1.0f, 1.0f, 1.0f};

    triangle0.indices.resize(6);
    triangle0.indices = {0, 1, 2, 2, 3, 0};

    triangle1.vertices.resize(4);
    triangle1.vertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}}, 
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}} 
    };

    triangle1.indices.resize(6);
    triangle1.indices = {0, 1, 2, 2, 3, 0};

    uploadMesh(triangle0);
    uploadMesh(triangle1);
}

void VulkanEngine::uploadMesh(Mesh &mesh) {
    //createVertexBuffer(mesh);
    //createIndexBuffer(mesh);
    //createVertexIndexBuffer(mesh);
    createVertexIndexBufferT(mesh);
}

void VulkanEngine::createVertexBuffer(Mesh &mesh) {
   // total size in BYTES of buffer allocating
    mesh.vertices_size = pad_uniform_buffer_size(sizeof(Vertex) * mesh.vertices.size());
    const size_t buffersize = mesh.vertices_size;

    // staging buffer (temporary on CPU)
    AllocatedBuffer staging_buffer = createBuffer(buffersize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );

    void *data;
    vmaMapMemory(m_allocator, staging_buffer.allocation, &data);
        memcpy(data, mesh.vertices.data(), buffersize);
    vmaUnmapMemory(m_allocator, staging_buffer.allocation);

    // locale buffer (on GPU)
    mesh.vertexBuffer = createBuffer(buffersize, 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );

    m_mainDeletionQueue.push_function([=]() {
        vmaDestroyBuffer(m_allocator, mesh.vertexBuffer.buffer, mesh.vertexBuffer.allocation);
    });

    copyBuffer(staging_buffer.buffer, mesh.vertexBuffer.buffer, buffersize, 0, 0);

    vmaDestroyBuffer(m_allocator, staging_buffer.buffer, staging_buffer.allocation);    
}

void VulkanEngine::createIndexBuffer(Mesh &mesh) {    
    mesh.indices_size = pad_uniform_buffer_size(sizeof(mesh.indices[0]) * mesh.indices.size());
    const size_t bufferSize = mesh.indices_size;

    // staging buffer
    AllocatedBuffer staging_buffer = createBuffer(bufferSize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );

    void *data;
    vmaMapMemory(m_allocator, staging_buffer.allocation, &data);
        memcpy(data, mesh.indices.data(), bufferSize); 
    vmaUnmapMemory(m_allocator, staging_buffer.allocation);

    // gpu buffer
    mesh.indexBuffer = createBuffer(bufferSize, 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );

    m_mainDeletionQueue.push_function([=]() {
        vmaDestroyBuffer(m_allocator, mesh.indexBuffer.buffer, mesh.indexBuffer.allocation);
    });

    // copy
    copyBuffer(staging_buffer.buffer, mesh.indexBuffer.buffer, bufferSize, 0, 0);

    vmaDestroyBuffer(m_allocator, staging_buffer.buffer, staging_buffer.allocation);
}

void VulkanEngine::createVertexIndexBuffer(Mesh &mesh) {
    mesh.vertices_size = pad_uniform_buffer_size(sizeof(Vertex) * mesh.vertices.size());
    mesh.indices_size = pad_uniform_buffer_size(sizeof(mesh.indices[0]) * mesh.indices.size());
    const size_t vertex_buffersize = mesh.vertices_size;
    const size_t index_buffersize = mesh.indices_size;
    const size_t total_buffersize = vertex_buffersize + index_buffersize;

    // staging
    AllocatedBuffer staging_buffer_vertex = createBuffer(vertex_buffersize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );

    // map
    void *vdata;
    vmaMapMemory(m_allocator, staging_buffer_vertex.allocation, &vdata);
        memcpy(vdata, mesh.vertices.data(), vertex_buffersize);   
    vmaUnmapMemory(m_allocator, staging_buffer_vertex.allocation);

    AllocatedBuffer staging_buffer_index = createBuffer(index_buffersize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );

    // map
    void *idata;
    vmaMapMemory(m_allocator, staging_buffer_index.allocation, &idata);
        memcpy(idata, mesh.indices.data(), index_buffersize);
    vmaUnmapMemory(m_allocator, staging_buffer_index.allocation);

    // gpu
    mesh.vertexIndexBuffer = createBuffer(total_buffersize, 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );

    m_mainDeletionQueue.push_function([=] () {
        vmaDestroyBuffer(m_allocator, mesh.vertexIndexBuffer.buffer, mesh.vertexIndexBuffer.allocation);
    });

    // copy
    copyBuffer(staging_buffer_index.buffer, mesh.vertexIndexBuffer.buffer, index_buffersize, 0, 0);
    copyBuffer(staging_buffer_vertex.buffer, mesh.vertexIndexBuffer.buffer, vertex_buffersize, 0, index_buffersize);
    
    // free
    vmaDestroyBuffer(m_allocator, staging_buffer_vertex.buffer, staging_buffer_vertex.allocation);
    vmaDestroyBuffer(m_allocator, staging_buffer_index.buffer, staging_buffer_index.allocation);
}

void VulkanEngine::createVertexIndexBufferT(Mesh &mesh) {
    mesh.vertices_size = pad_uniform_buffer_size(sizeof(Vertex) * mesh.vertices.size());
    mesh.indices_size = pad_uniform_buffer_size(sizeof(mesh.indices[0]) * mesh.indices.size());
    const size_t vertex_buffersize = mesh.vertices_size;
    const size_t index_buffersize = mesh.indices_size;
    const size_t total_buffersize = vertex_buffersize + index_buffersize;

    // staging
    AllocatedBuffer staging_buffer = createBuffer(total_buffersize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );

    // map
    char *data;
    vmaMapMemory(m_allocator, staging_buffer.allocation, (void **)&data);
        memcpy(data, mesh.indices.data(), index_buffersize);   
        data+=index_buffersize;
        memcpy(data, mesh.vertices.data(), vertex_buffersize);   
    vmaUnmapMemory(m_allocator, staging_buffer.allocation);

    // gpu
    mesh.vertexIndexBuffer = createBuffer(total_buffersize, 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );

    m_mainDeletionQueue.push_function([=] () {
        vmaDestroyBuffer(m_allocator, mesh.vertexIndexBuffer.buffer, mesh.vertexIndexBuffer.allocation);
    });

    // copy
    copyBuffer(staging_buffer.buffer, mesh.vertexIndexBuffer.buffer, total_buffersize, 0, 0);
    
    // free
    vmaDestroyBuffer(m_allocator, staging_buffer.buffer, staging_buffer.allocation);
}

size_t VulkanEngine::pad_uniform_buffer_size(size_t originalSize)
{
	// Calculate required alignment based on minimum device offset alignment
	size_t minUboAlignment = m_deviceProperties.limits.minUniformBufferOffsetAlignment;
	size_t alignedSize = originalSize;
	if (minUboAlignment > 0) {
		alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
	}
	return alignedSize;
}