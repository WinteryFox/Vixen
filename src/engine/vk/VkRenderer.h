#pragma once

#include "Device.h"
#include "VkPipeline.h"
#include "VkPipelineLayout.h"
#include "VkFramebuffer.h"
#include "VkSemaphore.h"
#include "VkBuffer.h"
#include "VkDescriptorSet.h"

namespace Vixen::Vk {
    class VkRenderer {
        std::shared_ptr<Device> device;

        Swapchain& swapchain;

        std::unique_ptr<VkPipelineLayout> pipelineLayout;

        std::shared_ptr<VkPipeline> pipeline;

        std::shared_ptr<VkCommandPool> renderCommandPool;

        std::vector<VkCommandBuffer> renderCommandBuffers;

        std::vector<VkImage> depthImages;

        std::vector<std::unique_ptr<VkImageView>> depthImageViews;

        std::vector<VkFramebuffer> framebuffers;

        std::vector<VkSemaphore> renderFinishedSemaphores;

    public:
        VkRenderer(
            const std::shared_ptr<Vk::Device>& device,
            Swapchain& swapchain,
            const std::shared_ptr<Vk::VkPipeline>& pipeline
        );

        VkRenderer(const VkRenderer&) = delete;

        VkRenderer& operator=(const VkRenderer&) = delete;

        ~VkRenderer();

        void render(
            const VkBuffer& buffer,
            uint32_t vertexCount,
            uint32_t indexCount,
            const VkDescriptorSet& set
        );

    private:
        void createFramebuffers();

        void prepare(
            VkCommandBuffer& commandBuffer,
            VkFramebuffer& framebuffer,
            const VkBuffer& buffer,
            uint32_t vertexCount,
            uint32_t indexCount,
            const VkDescriptorSet& set
        ) const;
    };
}
