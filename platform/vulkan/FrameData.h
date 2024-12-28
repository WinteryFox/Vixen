#pragma once
#include "commandbuffer/VulkanCommandBuffer.h"
#include "synchronization/VulkanSemaphore.h"

namespace Vixen {
    class VulkanCommandPool;
    class VulkanImageView;
    class VulkanImage2D;
    class VulkanDevice;

    struct FrameData {
        std::shared_ptr<VulkanDevice> device;

        std::shared_ptr<VulkanImage2D> resolveTarget;

        std::shared_ptr<VulkanImageView> resolveTargetView;

        std::shared_ptr<VulkanImage2D> colorTarget;

        std::shared_ptr<VulkanImageView> colorImageView;

        std::shared_ptr<VulkanImage2D> depthTarget;

        std::shared_ptr<VulkanImageView> depthImageView;

        std::shared_ptr<VulkanCommandPool> commandPool;

        VulkanCommandBuffer commandBuffer;

        VulkanSemaphore imageAvailableSemaphore;

        VulkanSemaphore renderFinishedSemaphore;
    };
}
