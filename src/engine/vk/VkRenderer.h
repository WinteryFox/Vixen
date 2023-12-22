#pragma once

#include "Device.h"
#include "VkPipeline.h"
#include "VkPipelineLayout.h"
#include "VkFramebuffer.h"
#include "VkSemaphore.h"
#include "VkBuffer.h"
#include "VkCommandPool.h"
#include "VkDescriptorSet.h"

namespace Vixen::Vk {
    class VkRenderer {
        std::shared_ptr<Device> device;

        std::shared_ptr<Swapchain> swapchain;

        std::unique_ptr<VkPipelineLayout> pipelineLayout;

        std::shared_ptr<VkPipeline> pipeline;

        std::shared_ptr<VkCommandPool> renderCommandPool;

        std::vector<VkCommandBuffer> renderCommandBuffers;

        std::vector<std::shared_ptr<VkImage>> depthImages;

        std::vector<std::unique_ptr<VkImageView>> depthImageViews;

        std::vector<VkFramebuffer> framebuffers;

        std::vector<VkSemaphore> renderFinishedSemaphores;

    public:
        VkRenderer(
            const std::shared_ptr<VkPipeline>& pipeline,
            const std::shared_ptr<Swapchain>& swapchain
        );

        VkRenderer(const VkRenderer&) = delete;

        VkRenderer& operator=(const VkRenderer&) = delete;

        ~VkRenderer();

        void render(
            const VkBuffer& buffer,
            uint32_t vertexCount,
            uint32_t indexCount,
            const std::vector<::VkDescriptorSet> &descriptorSets
        );

    private:
        void createFramebuffers();

        void prepare(
            VkCommandBuffer& commandBuffer,
            VkFramebuffer& framebuffer,
            const VkBuffer& buffer,
            uint32_t vertexCount,
            uint32_t indexCount,
            const std::vector<::VkDescriptorSet> &descriptorSets
        ) const;
    };
}
