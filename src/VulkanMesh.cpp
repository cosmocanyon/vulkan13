#include "VulkanMesh.h"

using namespace mii;

VertexInputDescription Vertex::getVertexInputDescription() {
    VertexInputDescription description {};

    VkVertexInputBindingDescription mainBinding {};
    mainBinding.binding = 0;
    mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    mainBinding.stride = sizeof(Vertex);
    
    description.bindings.push_back(mainBinding);

    VkVertexInputAttributeDescription attributePosition {};
    attributePosition.binding = 0;
    attributePosition.location = 0;
    attributePosition.format = VK_FORMAT_R32G32B32_SFLOAT;
    attributePosition.offset = offsetof(Vertex, position);

    VkVertexInputAttributeDescription attributeColor {};
    attributeColor.binding = 0;
    attributeColor.location = 1;
    attributeColor.format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeColor.offset = offsetof(Vertex, color);

    description.attributes.push_back(attributePosition);
    description.attributes.push_back(attributeColor);

    return description;
}

void Mesh::bind(VkCommandBuffer cmd) {
    VkBuffer vertexBuffers[] = {vertexIndexBuffer.buffer};
    VkDeviceSize offsets[] = {indices_size};
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmd, vertexIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
}

void Mesh::draw(VkCommandBuffer cmd) {
    vkCmdDrawIndexed(cmd, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}