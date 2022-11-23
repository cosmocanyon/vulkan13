#pragma once

#include "vk_types.h"
#include <glm/vec3.hpp>

#include <vector>

namespace mii {

struct VertexInputDescription {
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;

    static VertexInputDescription getVertexInputDescription();    
};

struct Mesh {
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;

    size_t indices_size, vertices_size;
    bool hasIndexBuffer = false;

    AllocatedBuffer vertexBuffer;
    AllocatedBuffer indexBuffer;
    AllocatedBuffer vertexIndexBuffer;

    void bind(VkCommandBuffer cmd);
    void draw(VkCommandBuffer cmd);
};

}   // namespace moo
