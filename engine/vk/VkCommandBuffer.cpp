#include "VkCommandBuffer.h"

namespace Vixen::Vk {
    VkCommandBuffer::VkCommandBuffer(::VkDevice device, ::VkCommandPool commandPool, ::VkCommandBuffer commandBuffer)
            : device(device),
              commandPool(commandPool),
              commandBuffer(commandBuffer),
              fence(device, true) {}

    VkCommandBuffer::~VkCommandBuffer() {
        fence.wait<void>(
                std::numeric_limits<uint64_t>::max(),
                [this](const auto &f) {
                    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
                }
        );
    }

    VkCommandBuffer &VkCommandBuffer::wait() {
        fence.wait<void>(
                std::numeric_limits<uint64_t>::max(),
                [](const auto &f) {}
        );

        return *this;
    }

    VkCommandBuffer &VkCommandBuffer::reset() {
        wait();
        checkVulkanResult(
                vkResetCommandBuffer(commandBuffer, 0),
                "Failed to reset command buffer"
        );

        return *this;
    }

    VkCommandBuffer &VkCommandBuffer::submit(
            ::VkQueue queue,
            const std::vector<::VkSemaphore> &waitSemaphores,
            const std::vector<::VkPipelineStageFlags> &waitMasks,
            const std::vector<::VkSemaphore> &signalSemaphores
    ) {
        fence.wait<void>(
                std::numeric_limits<uint64_t>::max(),
                [this, queue, waitSemaphores, waitMasks, signalSemaphores](const auto &f) {
                    fence.reset();

                    VkSubmitInfo info{
                            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,

                            .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
                            .pWaitSemaphores = waitSemaphores.data(),
                            .pWaitDstStageMask = waitMasks.data(),

                            .commandBufferCount = 1,
                            .pCommandBuffers = &commandBuffer,

                            .signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size()),
                            .pSignalSemaphores = signalSemaphores.data(),
                    };

                    checkVulkanResult(
                            vkQueueSubmit(queue, 1, &info, f),
                            "Failed to submit command buffer to queue"
                    );
                }
        );

        return *this;
    }
}
