#pragma once

#include "VulkanDevice.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/vec3.hpp"

namespace moo {

struct VulkanBuffer {
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
};

class VulkanModel {
private:
    VulkanDevice& m_device;

    VulkanBuffer m_vertexBuffer;
    uint32_t m_vertexCount;

    bool m_hasIndexBuffer = false;
    VulkanBuffer m_indexBuffer;
    uint32_t m_indexCount;

public:
    struct Vertex {
        glm::vec3 position;
        glm::vec3 color;

        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    struct Builder { // temporary helper object
        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};
    };

    VulkanModel(VulkanDevice &device, const Builder &builder);
    ~VulkanModel();

    VulkanModel(const VulkanModel&) = delete;
    VulkanModel &operator=(const VulkanModel&) = delete;

    void bind(VkCommandBuffer cmd);
    void draw(VkCommandBuffer cmd);

private:
    void createVertexBuffer(const std::vector<Vertex> &vertices);
    void createIndexBuffer(const std::vector<uint32_t> &indices);
};

}   // namespace moo