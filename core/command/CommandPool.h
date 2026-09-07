#pragma once

#include <cstdint>
#include <vector>

#include "CommandBufferType.h"
#include "CommandBuffer.h"

namespace Vixen {
    class CommandBuffer;

    struct CommandPool {
    private:
        friend class CommandBuffer;
        friend class RenderingDeviceDriver;
        friend class VulkanRenderingDeviceDriver;
        std::vector<CommandBuffer*> commandBuffers;
        std::vector<std::shared_ptr<std::atomic<CommandBuffer::State>>> commandSubmissions;

    public:
        CommandPool() = default;
        CommandPool(const CommandPool&) = delete;
        CommandPool& operator=(const CommandPool&) = delete;
        uint32_t queueFamily{};

        CommandBufferType type = CommandBufferType::Primary;

        virtual ~CommandPool();
    };
}
