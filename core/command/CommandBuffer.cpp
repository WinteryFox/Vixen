#include "CommandBuffer.h"

#include <algorithm>

#include "CommandPool.h"

namespace Vixen {
    CommandBuffer::CommandBuffer(QueueFamilyFlags capabilities, CommandPool* owner)
        : queueCapabilities(capabilities), pool(owner) {
        if (pool)
            pool->commandBuffers.push_back(this);
    }

    CommandBuffer::~CommandBuffer() {
        if (pool)
            std::erase(pool->commandBuffers, this);
    }

    void CommandBuffer::resetRecordingState() noexcept {
        state = State::Initial;
        submissionState.reset();
        dynamicStates = {};
        renderingState.reset();
        boundGraphicsPipeline = nullptr;
        boundComputePipeline = nullptr;
        vertexBindings.clear();
        indexBinding.reset();
    }

    CommandPool::~CommandPool() {
        for (auto* commandBuffer : commandBuffers) {
            commandBuffer->pool = nullptr;
            commandBuffer->resetRecordingState();
            commandBuffer->state = CommandBuffer::State::Invalid;
        }
    }
}
