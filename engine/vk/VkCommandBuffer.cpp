#include "VkCommandBuffer.h"

namespace Vixen::Vk {
    VkCommandBuffer::VkCommandBuffer(::VkDevice device, ::VkCommandPool commandPool, ::VkCommandBuffer commandBuffer)
            : device(device),
              commandPool(commandPool),
              commandBuffer(commandBuffer),
              fence(device, 1, true) {}

    VkCommandBuffer::~VkCommandBuffer() {
        fence.waitAll(std::numeric_limits<uint64_t>::max());
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void VkCommandBuffer::reset() {
        checkVulkanResult(
                vkResetCommandBuffer(commandBuffer, 0),
                "Failed to reset command buffer"
        );
    }

    ::VkCommandBuffer VkCommandBuffer::getCommandBuffer() const {
        return commandBuffer;
    }

    void VkCommandBuffer::submit(::VkQueue queue) {
        fence.waitAny<void>(
                std::numeric_limits<uint64_t>::max(),
                [this, &queue](const auto &f) {
                    fence.resetAll();

                    VkSubmitInfo info{
                            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,

                            .waitSemaphoreCount = 0,
                            .pWaitSemaphores = nullptr,
                            .pWaitDstStageMask = nullptr,

                            .commandBufferCount = 1,
                            .pCommandBuffers = &commandBuffer,

                            .signalSemaphoreCount = 0,
                            .pSignalSemaphores = nullptr,
                    };

                    checkVulkanResult(
                            vkQueueSubmit(queue, 1, &info, f),
                            "Failed to submit command buffer to queue"
                    );
                }
        );
    }
}
