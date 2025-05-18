#pragma once

namespace Vixen {
    struct VulkanCommandQueue final : CommandQueue {
        std::vector<VkSemaphore> imageSemaphores{};
        std::vector<Swapchain*> imageSemaphoresSwapchains{};
        std::vector<uint32_t> pendingSemaphoresForExecute{};
        std::vector<uint32_t> pendingSemaphoresForFence{};
        std::vector<uint32_t> freeImageSemaphores{};
        std::vector<std::pair<Fence*, uint32_t>> imageSemaphoresForFences{};
        uint32_t queueFamily = 0;
        uint32_t queueIndex = 0;
    };
}
