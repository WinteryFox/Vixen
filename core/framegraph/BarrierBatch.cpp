#include "BarrierBatch.h"

#include <utility>

#include "../RenderingDeviceDriver.h"

namespace Vixen {
    BarrierBatch& BarrierBatcher::getBatch(
        const PipelineStageFlags sourceStages,
        const PipelineStageFlags destinationStages
    ) {
        for (auto& batch : batches)
            if (batch.sourceStages == sourceStages &&
                batch.destinationStages == destinationStages)
                return batch;

        return batches.emplace_back(BarrierBatch{
            .sourceStages = sourceStages,
            .destinationStages = destinationStages,
            .memoryBarriers = {},
            .bufferBarriers = {},
            .imageBarriers = {}
        });
    }

    void BarrierBatcher::addMemoryBarrier(
        const PipelineStageFlags sourceStages,
        const PipelineStageFlags destinationStages,
        const MemoryBarrier barrier
    ) {
        getBatch(sourceStages, destinationStages).memoryBarriers.push_back(barrier);
    }

    void BarrierBatcher::addBufferBarrier(
        const BufferState& source,
        const BufferState& destination,
        BufferBarrier barrier
    ) {
        barrier.sourceAccess = source.access;
        barrier.destinationAccess = destination.access;
        getBatch(source.stages, destination.stages).bufferBarriers.push_back(barrier);
    }

    void BarrierBatcher::addImageBarrier(
        const ImageState& source,
        const ImageState& destination,
        ImageBarrier barrier
    ) {
        barrier.sourceAccess = source.access;
        barrier.destinationAccess = destination.access;
        barrier.oldLayout = source.layout;
        barrier.newLayout = destination.layout;
        getBatch(source.stages, destination.stages).imageBarriers.push_back(barrier);
    }

    std::span<const BarrierBatch> BarrierBatcher::getBatches() const noexcept {
        return batches;
    }

    std::vector<BarrierBatch> BarrierBatcher::takeBatches() && noexcept {
        return std::move(batches);
    }

    void BarrierBatcher::clear() noexcept {
        batches.clear();
    }

    void emitBarrierBatches(
        RenderingDeviceDriver& driver,
        CommandBuffer* commandBuffer,
        const std::span<const BarrierBatch> batches
    ) {
        for (const auto& batch : batches)
            driver.commandPipelineBarrier(
                commandBuffer,
                batch.sourceStages,
                batch.destinationStages,
                batch.memoryBarriers,
                batch.bufferBarriers,
                batch.imageBarriers
            );
    }
}
