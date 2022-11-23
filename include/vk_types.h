// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

namespace mii {

//we will add our main reusable types here
struct AllocatedBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
};

}   // namespace mii