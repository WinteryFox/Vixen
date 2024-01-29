#pragma once

#include "Device.h"
#include "VkCommandPool.h"
#include "VkMesh.h"
#include "VkPipeline.h"
#include "VkPipelineLayout.h"
#include "VkSemaphore.h"

namespace Vixen::Vk {
    class Swapchain;

    class VkRenderer {
        std::shared_ptr<Device> device;

        std::shared_ptr<Swapchain> swapchain;

        std::unique_ptr<VkPipelineLayout> pipelineLayout;

        std::shared_ptr<VkPipeline> pipeline;

        std::shared_ptr<VkCommandPool> renderCommandPool;

        std::vector<VkCommandBuffer> renderCommandBuffers;

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
            const VkMesh& mesh,
            const std::vector<::VkDescriptorSet>& descriptorSets
        );

    private:
        void prepare(
            uint32_t imageIndex,
            const VkCommandBuffer& commandBuffer,
            const VkMesh& mesh,
            const std::vector<::VkDescriptorSet>& descriptorSets
        ) const;
    };
}
