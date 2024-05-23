#include "VulkanMesh.h"

#include "buffer/VulkanBuffer.h"
#include "commandbuffer/CommandBufferLevel.h"
#include "commandbuffer/CommandBufferUsage.h"
#include "commandbuffer/VulkanCommandPool.h"
#include "core/BufferUsage.h"
#include "core/IndexFormat.h"
#include "device/VulkanDevice.h"

namespace Vixen {
    VulkanMesh::VulkanMesh(const std::shared_ptr<VulkanDevice> &device)
        : device(device),
          vertexBuffer(nullptr),
          vertexCount(0),
          indexBuffer(nullptr),
          indexCount(0),
          indexFormat(),
          topology() {}

    uint32_t VulkanMesh::getVertexCount() const {
        return vertexCount;
    }

    void VulkanMesh::setIndices(const std::vector<uint16_t> &indices, const PrimitiveTopology topology) {
        this->indexFormat = IndexFormat::UnsignedInt16;
        this->topology = topology;
        this->indexCount = indices.size();
        indexBuffer = std::make_unique<VulkanBuffer>(
            device,
            BufferUsage::Index | BufferUsage::CopyDestination,
            indexCount,
            sizeof(uint16_t)
        );
        upload(*indexBuffer, std::bit_cast<const std::byte *>(indices.data()));
    }

    void VulkanMesh::setIndices(const std::vector<uint32_t> &indices, PrimitiveTopology topology) {
        this->indexFormat = IndexFormat::UnsignedInt32;
        this->topology = topology;
        this->indexCount = indices.size();
        indexBuffer = std::make_unique<VulkanBuffer>(
            device,
            BufferUsage::Index | BufferUsage::CopyDestination,
            indexCount,
            sizeof(uint32_t)
        );
        upload(*indexBuffer, std::bit_cast<const std::byte *>(indices.data()));
    }

    void VulkanMesh::setVertices(const std::vector<Vertex> &vertices) {
        this->vertexCount = vertices.size();
        vertexBuffer = std::make_unique<VulkanBuffer>(
            device,
            BufferUsage::Vertex | BufferUsage::CopyDestination,
            vertexCount,
            sizeof(Vertex)
        );
        upload(*vertexBuffer, std::bit_cast<const std::byte *>(vertices.data()));
    }

    void VulkanMesh::setMaterial(const std::shared_ptr<const Material> &material) {
        this->material = material;
    }

    void VulkanMesh::upload(const VulkanBuffer &destination, const std::byte *data) const {
        const auto &stagingBuffer = VulkanBuffer(device, BufferUsage::CopySource, destination.getCount(),
                                           destination.getStride());
        stagingBuffer.setData(data);

        const auto &cmd = device->getTransferCommandPool()->allocate(CommandBufferLevel::Primary);
        cmd.begin(CommandBufferUsage::Once);
        cmd.copyBuffer(stagingBuffer, destination);
        cmd.end();
        cmd.submit(device->getTransferQueue(), {}, {}, {});
    }

    uint32_t VulkanMesh::getIndexCount() const { return indexCount; }

    IndexFormat VulkanMesh::getIndexFormat() const { return indexFormat; }

    PrimitiveTopology VulkanMesh::getTopology() const { return topology; }

    const VulkanBuffer &VulkanMesh::getVertexBuffer() const { return *vertexBuffer; }

    const VulkanBuffer &VulkanMesh::getIndexBuffer() const { return *indexBuffer; }

    std::shared_ptr<const Material> VulkanMesh::getMaterial() const { return material; }
}
