#pragma once

#include "Device.h"
#include "Pipeline.h"
#include "CommandBuffer.h"

namespace Vixen::Vk {
    class Renderer {
        std::shared_ptr<Vk::Device> device;

        std::unique_ptr<Vk::Pipeline> pipeline;

        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

        std::vector<CommandBuffer> commandBuffers;

    public:
        Renderer(const std::shared_ptr<Vk::Device> &device, std::unique_ptr<Vk::Pipeline> &&pipeline);

        Renderer(const Renderer &) = delete;

        Renderer &operator=(const Renderer &) = delete;

        ~Renderer();
    };
}
