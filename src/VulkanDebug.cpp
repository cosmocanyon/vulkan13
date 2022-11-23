#include "VulkanDebug.h"

#include <string>
#include <iostream>
#include <sstream>
#include <cassert>

using namespace moo;

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebug::debugUtilsMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    // Select prefix depending on flags passed to the callback
    std::string prefix("");

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        prefix = "VERBOSE: ";
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        prefix = "INFO: ";
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        prefix = "WARNING: ";
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        prefix = "ERROR: ";
    }

    // Display message to default output (console/logcat)
    std::stringstream debugMessage;
    debugMessage << prefix << "[" << pCallbackData->messageIdNumber << "][" << pCallbackData->pMessageIdName << "] : " << pCallbackData->pMessage;

#if defined(__ANDROID__)
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        LOGE("%s", debugMessage.str().c_str());
    } else {
        LOGD("%s", debugMessage.str().c_str());
    }
#else
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        std::cerr << debugMessage.str() << "\n";
    } else {
        std::cout << debugMessage.str() << "\n";
    }
    fflush(stdout);
#endif

    // The return value of this callback controls whether the Vulkan call that caused the validation message will be aborted or not
    // We return VK_FALSE as we DON'T want Vulkan calls that cause a validation message to abort
    // If you instead want to have calls abort, pass in VK_TRUE and the function will return VK_ERROR_VALIDATION_FAILED_EXT 
    return VK_FALSE;
}
    
void VulkanDebug::setupDebugging(VkInstance instance) {
    // setup pointers to the VK_EXT_debug_utils commands
    CreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    DestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

    // create a debug utils messenger will trigger debugUtilsMessengerCallback
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
    debugMessenger_cInfo.pfnUserCallback = debugUtilsMessengerCallback; 
    
    VkResult result = CreateDebugUtilsMessengerEXT(instance, &debugMessenger_cInfo, nullptr, &m_debugMessenger);
    assert(result == VK_SUCCESS);
}

void VulkanDebug::freeDebugCallback(VkInstance instance) {
    if (m_debugMessenger != VK_NULL_HANDLE) {
        DestroyDebugUtilsMessengerEXT(instance, m_debugMessenger, nullptr);
    }
}