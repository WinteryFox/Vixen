#pragma once

#include "Device.h"
#include "VkPipeline.h"
#include "CommandBuffer.h"
#include "VkPipelineLayout.h"
#include "VkCommandPool.h"

namespace Vixen::Vk {
    class VkRenderer {
        std::shared_ptr<Device> device;

        std::unique_ptr<VkPipelineLayout> pipelineLayout;

        std::unique_ptr<VkPipeline> pipeline;

        std::vector<CommandBuffer> commandBuffers;

        VkCommandPool renderPool;

    public:
        VkRenderer(const std::shared_ptr<Vk::Device> &device, std::unique_ptr<Vk::VkPipeline> &pipeline);

        VkRenderer(const VkRenderer &) = delete;

        VkRenderer &operator=(const VkRenderer &) = delete;

        ~VkRenderer();
    };
}
