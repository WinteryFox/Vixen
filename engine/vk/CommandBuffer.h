#pragma once

#include "Device.h"

namespace Vixen::Vk {
    class CommandBuffer {
        const std::shared_ptr<Vk::Device> device;

        VkCommandBuffer buffer{};

    public:
        CommandBuffer(const std::shared_ptr<Vk::Device> &device);

        CommandBuffer(const CommandBuffer &) = delete;

        CommandBuffer &operator=(const CommandBuffer &) = delete;

        ~CommandBuffer();
    };
}
