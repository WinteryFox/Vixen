#pragma once

#include <cstdint>
#include <atomic>
#include <memory>
#include <optional>
#include <vector>

#include "command/QueueFamilyFlags.h"
#include "image/ImageDataFormat.h"
#include "image/ImageSamples.h"
#include "pipeline/DynamicStateFlags.h"
#include "rendering/IndexFormat.h"

namespace Vixen {
    class GraphicsPipeline;
    class ComputePipeline;
    class Buffer;
    struct CommandPool;

    class CommandBuffer {
        friend class RenderingDeviceDriver;
        friend class VulkanRenderingDeviceDriver;
        friend struct CommandPool;
        friend struct VulkanCommandQueue;
        friend struct VulkanFence;

        enum class State {
            Initial,
            Recording,
            Executable,
            Pending,
            Invalid
        };

        struct RenderingState {
            std::vector<ImageDataFormat> colorFormats{};

            std::optional<ImageDataFormat> depthStencilFormat = std::nullopt;

            ImageSamples samples = ImageSamples::One;
        };

        struct VertexBufferBinding {
            const Buffer* buffer = nullptr;
            uint64_t offset = 0;
        };

        struct IndexBufferBinding {
            const Buffer* buffer = nullptr;
            uint64_t offset = 0;
            IndexFormat format = IndexFormat::UnsignedInt16;
        };

        QueueFamilyFlags queueCapabilities{};

        DynamicStateFlags dynamicStates{};

        State state = State::Initial;

        // Shared with submission/fence tracking without extending the wrapper's lifetime.
        std::shared_ptr<std::atomic<State>> submissionState;
        CommandPool* pool = nullptr;

        [[nodiscard]] State getState() const noexcept {
            return state == State::Pending && submissionState ? submissionState->load() : state;
        }

        void resetRecordingState() noexcept;

        std::optional<RenderingState> renderingState{};

        const GraphicsPipeline* boundGraphicsPipeline = nullptr;

        const ComputePipeline* boundComputePipeline = nullptr;

        std::vector<VertexBufferBinding> vertexBindings{};

        std::optional<IndexBufferBinding> indexBinding{};

    protected:
        explicit CommandBuffer(
            QueueFamilyFlags queueCapabilities,
            CommandPool* pool = nullptr
        );

    public:
        CommandBuffer(const CommandBuffer&) = delete;
        CommandBuffer& operator=(const CommandBuffer&) = delete;

        CommandBuffer(CommandBuffer&&) = delete;
        CommandBuffer& operator=(CommandBuffer&&) = delete;

        virtual ~CommandBuffer();
    };
}
