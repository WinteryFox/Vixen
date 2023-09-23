#pragma once

#include "Device.h"
#include "VkPipeline.h"
#include "CommandBuffer.h"
#include "VkPipelineLayout.h"
#include "VkCommandPool.h"
#include "VkCommandBuffer.h"

namespace Vixen::Vk {
    class VkRenderer {
        std::shared_ptr<Device> device;

        const Swapchain &swapchain;

        std::unique_ptr<VkPipelineLayout> pipelineLayout;

        std::unique_ptr<VkPipeline> pipeline;

        std::vector<CommandBuffer> commandBuffers;

        std::shared_ptr<VkCommandPool> renderCommandPool;

        std::vector<VkCommandBuffer> renderCommandBuffers;

    public:
        VkRenderer(
                const std::shared_ptr<Vk::Device> &device,
                const Swapchain &swapchain,
                std::unique_ptr<Vk::VkPipeline> &pipeline
        );

        VkRenderer(const VkRenderer &) = delete;

        VkRenderer &operator=(const VkRenderer &) = delete;

        ~VkRenderer();
    };
}
