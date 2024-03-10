#include "VkMesh.h"

#include "Device.h"
#include "../IndexFormat.h"
#include "../PrimitiveTopology.h"

namespace Vixen::Vk {
    VkMesh::VkMesh(const std::shared_ptr<Device>& device)
        : device(device),
          vertexCount(0),
          indexCount(0),
          indexFormat(),
          topology() {}

    uint32_t VkMesh::getVertexCount() const {
        return vertexCount;
    }

    void VkMesh::setIndices(const std::vector<uint16_t>& indices, const PrimitiveTopology topology) {
        this->indexFormat = IndexFormat::UnsignedInt16;
        this->topology = topology;
        this->indexCount = indices.size();
        indexBuffer = VkBuffer(
            device,
            BufferUsage::Index | BufferUsage::CopyDestination,
            indexCount,
            sizeof(uint16_t)
        );
        upload(indexBuffer, reinterpret_cast<const std::byte*>(indices.data()));
    }

    void VkMesh::setIndices(const std::vector<uint32_t>& indices, PrimitiveTopology topology) {
        this->indexFormat = IndexFormat::UnsignedInt32;
        this->topology = topology;
        this->indexCount = indices.size();
        indexBuffer = VkBuffer(
            device,
            BufferUsage::Index | BufferUsage::CopyDestination,
            indexCount,
            sizeof(uint32_t)
        );
        upload(indexBuffer, reinterpret_cast<const std::byte*>(indices.data()));
    }

    void VkMesh::setVertices(const std::vector<Vertex>& vertices) {
        this->vertexCount = vertices.size();
        vertexBuffer = VkBuffer(
            device,
            BufferUsage::Vertex | BufferUsage::CopyDestination,
            vertexCount,
            sizeof(Vertex)
        );
        upload(vertexBuffer, reinterpret_cast<const std::byte*>(vertices.data()));
    }

    void VkMesh::setMaterial(const std::shared_ptr<const Material>& material) {
        this->material = material;
    }

    void VkMesh::upload(const VkBuffer& destination, const std::byte* data) const {
        const auto& stagingBuffer = VkBuffer(device, BufferUsage::CopySource, destination.getCount(),
                                             destination.getStride());
        stagingBuffer.setData(data);

        const auto& cmd = device->getTransferCommandPool()
                                ->allocate(CommandBufferLevel::Primary);
        cmd.begin(CommandBufferUsage::Once);
        cmd.copyBuffer(stagingBuffer, destination);
        cmd.end();
        cmd.submit(device->getTransferQueue(), {}, {}, {});
    }

    uint32_t VkMesh::getIndexCount() const { return indexCount; }

    IndexFormat VkMesh::getIndexFormat() const { return indexFormat; }

    PrimitiveTopology VkMesh::getTopology() const { return topology; }

    const VkBuffer& VkMesh::getVertexBuffer() const { return vertexBuffer; }

    const VkBuffer& VkMesh::getIndexBuffer() const { return indexBuffer; }

    std::shared_ptr<const Material> VkMesh::getMaterial() const { return material; }
}
