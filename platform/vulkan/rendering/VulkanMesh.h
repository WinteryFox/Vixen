#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

namespace Vixen {
    class VulkanDevice;
    enum class VertexAttribute;
    enum class PrimitiveTopology;
    enum class IndexFormat;
}

namespace Vixen {
    class VulkanBuffer;
    struct Material;

    struct Vertex {
        glm::vec3 position;
        glm::vec4 color;
        glm::vec2 uv;
        glm::vec3 normal;
    };

    class VulkanMesh {
        std::shared_ptr<VulkanDevice> device;

        std::unique_ptr<VulkanBuffer> vertexBuffer;

        uint32_t vertexCount;

        std::unordered_map<VertexAttribute, size_t> vertexBufferOffsets;

        std::unordered_map<VertexAttribute, VulkanBuffer> hostBuffers;

        std::unique_ptr<VulkanBuffer> indexBuffer;

        uint32_t indexCount;

        IndexFormat indexFormat;

        PrimitiveTopology topology;

        std::shared_ptr<const Material> material;

    public:
        explicit VulkanMesh(const std::shared_ptr<VulkanDevice> &device);

        [[nodiscard]] uint32_t getVertexCount() const;

        [[nodiscard]] uint32_t getIndexCount() const;

        [[nodiscard]] IndexFormat getIndexFormat() const;

        [[nodiscard]] PrimitiveTopology getTopology() const;

        const VulkanBuffer &getVertexBuffer() const;

        const VulkanBuffer &getIndexBuffer() const;

        std::shared_ptr<const Material> getMaterial() const;

        void setIndices(const std::vector<uint16_t> &indices, PrimitiveTopology topology);

        void setIndices(const std::vector<uint32_t> &indices, PrimitiveTopology topology);

        void setVertices(const std::vector<Vertex> &vertices);

        void setMaterial(const std::shared_ptr<const Material> &material);

    private:
        void upload(const VulkanBuffer &destination, const std::byte *data) const;
    };
}
