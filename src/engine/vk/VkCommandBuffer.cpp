#include "VkCommandBuffer.h"

#include "VkCommandPool.h"

namespace Vixen::Vk {
    VkCommandBuffer::VkCommandBuffer(
        const std::shared_ptr<VkCommandPool>& commandPool,
        ::VkCommandBuffer commandBuffer
    ) : commandPool(commandPool),
        commandBuffer(commandBuffer),
        fence(commandPool->getDevice(), true) {}

    VkCommandBuffer::VkCommandBuffer(VkCommandBuffer&& other) noexcept
        : commandPool(std::exchange(other.commandPool, nullptr)),
          commandBuffer(std::exchange(other.commandBuffer, nullptr)),
          fence(other.commandPool->getDevice(), true) {}

    VkCommandBuffer& VkCommandBuffer::operator=(VkCommandBuffer&& other) noexcept {
        std::swap(commandBuffer, other.commandBuffer);
        std::swap(commandBuffer, other.commandBuffer);
        std::swap(fence, other.fence);

        return *this;
    }

    VkCommandBuffer::~VkCommandBuffer() {
        fence.wait();
        vkFreeCommandBuffers(commandPool->getDevice()->getDevice(), commandPool->getCommandPool(), 1, &commandBuffer);
    }

    VkCommandBuffer& VkCommandBuffer::wait() {
        fence.wait();

        return *this;
    }

    VkCommandBuffer& VkCommandBuffer::reset() {
        wait();
        checkVulkanResult(
            vkResetCommandBuffer(commandBuffer, 0),
            "Failed to reset command buffer"
        );

        return *this;
    }

    VkCommandBuffer& VkCommandBuffer::begin(Usage usage) {
        const VkCommandBufferBeginInfo info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = static_cast<VkCommandBufferUsageFlags>(usage)
        };

        checkVulkanResult(
            vkBeginCommandBuffer(commandBuffer, &info),
            "Failed to begin command buffer"
        );

        return *this;
    }

    VkCommandBuffer& VkCommandBuffer::end() {
        checkVulkanResult(
            vkEndCommandBuffer(commandBuffer),
            "Failed to end command buffer"
        );

        return *this;
    }

    VkCommandBuffer& VkCommandBuffer::copyBuffer(const VkBuffer& source, const VkBuffer& destination) {
        const VkBufferCopy region{
            .srcOffset = 0,
            .dstOffset = 0,
            .size = source.getSize(),
        };

        vkCmdCopyBuffer(commandBuffer, source.getBuffer(), destination.getBuffer(), 1, &region);

        return *this;
    }

    VkCommandBuffer& VkCommandBuffer::copyImage(const VkImage& source, const VkImage& destination) {
        const VkImageCopy region{
            .srcSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .srcOffset = 0,
            .dstSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .dstOffset = 0,
            .extent = {
                .width = source.getWidth(),
                .height = source.getHeight(),
                .depth = 1
            }
        };

        vkCmdCopyImage(commandBuffer, source.getImage(), source.getLayout(), destination.getImage(),
                       destination.getLayout(), 1, &region);

        return *this;
    }

    VkCommandBuffer& VkCommandBuffer::copyBufferToImage(const VkBuffer& source, const VkImage& destination) {
        const VkBufferImageCopy region{
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .imageOffset{
                .x = 0,
                .y = 0,
                .z = 0
            },
            .imageExtent{
                .width = destination.getWidth(),
                .height = destination.getHeight(),
                .depth = 1
            }
        };

        vkCmdCopyBufferToImage(commandBuffer, source.getBuffer(), destination.getImage(), destination.getLayout(), 1,
                               &region);

        return *this;
    }

    void VkCommandBuffer::submit(
        ::VkQueue queue,
        const std::vector<::VkSemaphore>& waitSemaphores,
        const std::vector<::VkPipelineStageFlags>& waitMasks,
        const std::vector<::VkSemaphore>& signalSemaphores
    ) {
        fence.wait();
        fence.reset();

        const VkSubmitInfo info{
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
            vkQueueSubmit(queue, 1, &info, fence.getFence()),
            "Failed to submit command buffer to queue"
        );
    }
}
