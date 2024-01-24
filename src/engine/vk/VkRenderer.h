#pragma once

#include "Device.h"
#include "VkBuffer.h"
#include "VkCommandPool.h"
#include "VkFramebuffer.h"
#include "VkMesh.h"
#include "VkPipeline.h"
#include "VkPipelineLayout.h"
#include "VkSemaphore.h"

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
            const VkMesh &mesh,
            const std::vector<::VkDescriptorSet>& descriptorSets
        );

    private:
        void createFramebuffers();

        void prepare(
            VkCommandBuffer& commandBuffer,
            VkFramebuffer& framebuffer,
            const VkMesh &mesh,
            const std::vector<::VkDescriptorSet>& descriptorSets
        ) const;
    };
}
