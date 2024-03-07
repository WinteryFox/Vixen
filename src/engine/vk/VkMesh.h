#pragma once

#include "VkBuffer.h"
#include "../VertexAttribute.h"

namespace Vixen {
    enum class PrimitiveTopology;
    enum class IndexFormat;
}

namespace Vixen::Vk {
    struct Material;

    struct Vertex {
        glm::vec3 position;
        glm::vec4 color;
        glm::vec2 uv;
    };

    class VkMesh {
        std::shared_ptr<Device> device;

        VkBuffer vertexBuffer;

        uint32_t vertexCount;

        std::unordered_map<VertexAttribute, size_t> vertexBufferOffsets;

        std::unordered_map<VertexAttribute, VkBuffer> hostBuffers;

        VkBuffer indexBuffer;

        uint32_t indexCount;

        IndexFormat indexFormat;

        PrimitiveTopology topology;

        std::shared_ptr<const Material> material;

    public:
        explicit VkMesh(const std::shared_ptr<Device>& device);

        [[nodiscard]] uint32_t getVertexCount() const;

        [[nodiscard]] uint32_t getIndexCount() const;

        [[nodiscard]] IndexFormat getIndexFormat() const;

        [[nodiscard]] PrimitiveTopology getTopology() const;

        const VkBuffer& getVertexBuffer() const;

        const VkBuffer& getIndexBuffer() const;

        std::shared_ptr<const Material> getMaterial() const;

        void setIndices(const std::vector<uint16_t>& indices, PrimitiveTopology topology);

        void setIndices(const std::vector<uint32_t>& indices, PrimitiveTopology topology);

        void setVertices(const std::vector<Vertex>& vertices);

        void setMaterial(const std::shared_ptr<const Material> &material);

    private:
        void upload(const VkBuffer& destination, const std::byte* data) const;
    };
}
