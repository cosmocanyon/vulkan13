#pragma once

#include "vulkan/vulkan.h"

namespace moo {

struct VulkanDebug {
	VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

	PFN_vkCreateDebugUtilsMessengerEXT CreateDebugUtilsMessengerEXT = VK_NULL_HANDLE;
    PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugUtilsMessengerEXT = VK_NULL_HANDLE;

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    );

    // load debug function pointers and set debug callback
    // if callback NULL, default message callback will be used
    void setupDebugging(VkInstance instance);
    void freeDebugCallback(VkInstance instance);
};

}   // namespace moo