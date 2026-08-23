#pragma once

#include <span>
#include <vector>

#include "Resource.h"
#include "../BufferBarrier.h"
#include "../ImageBarrier.h"
#include "../MemoryBarrier.h"

namespace Vixen {
    class RenderingDeviceDriver;
    struct CommandBuffer;

    struct BarrierBatch {
        PipelineStageFlags sourceStages;
        PipelineStageFlags destinationStages;
        std::vector<MemoryBarrier> memoryBarriers;
        std::vector<BufferBarrier> bufferBarriers;
        std::vector<ImageBarrier> imageBarriers;
    };

    class BarrierBatcher {
        std::vector<BarrierBatch> batches;

        BarrierBatch& getBatch(
            PipelineStageFlags sourceStages,
            PipelineStageFlags destinationStages
        );

    public:
        void addMemoryBarrier(
            PipelineStageFlags sourceStages,
            PipelineStageFlags destinationStages,
            MemoryBarrier barrier
        );

        void addBufferBarrier(
            const BufferState& source,
            const BufferState& destination,
            BufferBarrier barrier
        );

        void addImageBarrier(
            const ImageState& source,
            const ImageState& destination,
            ImageBarrier barrier
        );

        [[nodiscard]] std::span<const BarrierBatch> getBatches() const noexcept;

        void clear() noexcept;
    };

    void emitBarrierBatches(
        RenderingDeviceDriver& driver,
        CommandBuffer* commandBuffer,
        std::span<const BarrierBatch> batches
    );
}
