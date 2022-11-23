#include "VulkanModel.h"

#include <cassert>

using namespace moo;

VulkanModel::VulkanModel(VulkanDevice &device, const Builder &builder) : m_device{device} {
    createVertexBuffer(builder.vertices);
    createIndexBuffer(builder.indices);
}

VulkanModel::~VulkanModel() {
    vkDestroyBuffer(m_device.getDevice(), m_vertexBuffer.buffer, nullptr);
    vkFreeMemory(m_device.getDevice(), m_vertexBuffer.bufferMemory, nullptr);

    if (m_hasIndexBuffer) {
        vkDestroyBuffer(m_device.getDevice(), m_indexBuffer.buffer, nullptr);
        vkFreeMemory(m_device.getDevice(), m_indexBuffer.bufferMemory, nullptr);
    }
}

void VulkanModel::createVertexBuffer(const std::vector<Vertex> &vertices) {
    m_vertexCount = static_cast<uint32_t>(vertices.size());
    assert(m_vertexCount >= 3 && "Vertex count must be at least 3.");

    VkDeviceSize bufferSize = sizeof(Vertex) * m_vertexCount;

// host buffer: staging buffer (on CPU)
    VulkanBuffer stagingBuffer;
    m_device.createBuffer(bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer.buffer, stagingBuffer.bufferMemory
    );

    void *data;
    vkMapMemory(m_device.getDevice(), stagingBuffer.bufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(m_device.getDevice(), stagingBuffer.bufferMemory);

// device buffer (on GPU)
    m_device.createBuffer(bufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_vertexBuffer.buffer, m_vertexBuffer.bufferMemory
    );

// copy from staging buffer to device buffer
    m_device.copyBuffer(stagingBuffer.buffer, m_vertexBuffer.buffer, bufferSize);

// free staging buffer
    vkDestroyBuffer(m_device.getDevice(), stagingBuffer.buffer, nullptr);
    vkFreeMemory(m_device.getDevice(), stagingBuffer.bufferMemory, nullptr);
}

void VulkanModel::createIndexBuffer(const std::vector<uint32_t> &indices) {
    m_indexCount = static_cast<uint32_t>(indices.size());
    m_hasIndexBuffer = m_indexCount > 0;

    if(!m_hasIndexBuffer) {
        return;
    }

    VkDeviceSize bufferSize = sizeof(indices[0]) * m_indexCount;

    // host buffer: staging buffer (on CPU)
    VulkanBuffer stagingBuffer;
    m_device.createBuffer(bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer.buffer, stagingBuffer.bufferMemory
    );

    void *data;
    vkMapMemory(m_device.getDevice(), stagingBuffer.bufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(m_device.getDevice(), stagingBuffer.bufferMemory);

// device buffer (on GPU)
    m_device.createBuffer(bufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_indexBuffer.buffer, m_indexBuffer.bufferMemory
    );

// copy from staging buffer to device buffer
    m_device.copyBuffer(stagingBuffer.buffer, m_indexBuffer.buffer, bufferSize);

// free staging buffer
    vkDestroyBuffer(m_device.getDevice(), stagingBuffer.buffer, nullptr);
    vkFreeMemory(m_device.getDevice(), stagingBuffer.bufferMemory, nullptr);
}

void VulkanModel::bind(VkCommandBuffer cmd) {
    VkBuffer vertexBuffers[] = {m_vertexBuffer.buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

    if(m_hasIndexBuffer) {
        vkCmdBindIndexBuffer(cmd, m_indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    }
}

void VulkanModel::draw(VkCommandBuffer cmd) {
    if (m_hasIndexBuffer) {
        vkCmdDrawIndexed(cmd, m_indexCount, 1, 0, 0, 0);
    } else {
        vkCmdDraw(cmd, m_vertexCount, 1, 0, 0);
    }    
}

std::vector<VkVertexInputBindingDescription> VulkanModel::Vertex::getBindingDescriptions() {
    std::vector<VkVertexInputBindingDescription> binding_desc(1);

    binding_desc[0].binding = 0;
    binding_desc[0].stride = sizeof(VulkanModel::Vertex);
    binding_desc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    return binding_desc;
}

std::vector<VkVertexInputAttributeDescription> VulkanModel::Vertex::getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attribute_desc(2);

    attribute_desc[0].binding = 0;
    attribute_desc[0].location = 0;
    attribute_desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_desc[0].offset = offsetof(VulkanModel::Vertex, position);

    attribute_desc[1].binding = 0;
    attribute_desc[1].location = 1;
    attribute_desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_desc[1].offset = offsetof(VulkanModel::Vertex, color);

    return attribute_desc;
}