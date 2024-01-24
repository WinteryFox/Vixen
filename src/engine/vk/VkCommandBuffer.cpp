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
          fence(std::move(other.fence)) {}

    VkCommandBuffer& VkCommandBuffer::operator=(VkCommandBuffer&& other) noexcept {
        std::swap(commandPool, other.commandPool);
        std::swap(commandBuffer, other.commandBuffer);
        std::swap(fence, other.fence);

        return *this;
    }

    VkCommandBuffer::~VkCommandBuffer() {
        fence.wait();
        vkFreeCommandBuffers(
            commandPool->getDevice()->getDevice(),
            commandPool->getCommandPool(),
            1,
            &commandBuffer
        );
    }

    void VkCommandBuffer::wait() const {
        fence.wait();
    }

    void VkCommandBuffer::reset() const {
        wait();
        checkVulkanResult(
            vkResetCommandBuffer(commandBuffer, 0),
            "Failed to reset command buffer"
        );
    }

    void VkCommandBuffer::begin(const CommandBufferUsage usage) const {
        VkCommandBufferBeginInfo info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
        };

        switch (usage) {
        case CommandBufferUsage::ONCE:
            info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            break;
        case CommandBufferUsage::SIMULTANEOUS:
            info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            break;
        case CommandBufferUsage::RENDER_PASS_CONTINUE:
            info.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
            break;
        }

        checkVulkanResult(
            vkBeginCommandBuffer(commandBuffer, &info),
            "Failed to begin command buffer"
        );
    }

    void VkCommandBuffer::end() const {
        checkVulkanResult(
            vkEndCommandBuffer(commandBuffer),
            "Failed to end command buffer"
        );
    }

    void VkCommandBuffer::submit(
        ::VkQueue queue,
        const std::vector<::VkSemaphore>& waitSemaphores,
        const std::vector<::VkPipelineStageFlags>& waitMasks,
        const std::vector<::VkSemaphore>& signalSemaphores
    ) const {
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

    void VkCommandBuffer::copyBuffer(const VkBuffer& source, const VkBuffer& destination) const {
        const VkBufferCopy region{
            .srcOffset = 0,
            .dstOffset = 0,
            .size = source.getSize(),
        };

        vkCmdCopyBuffer(commandBuffer, source.getBuffer(), destination.getBuffer(), 1, &region);
    }

    void VkCommandBuffer::copyBufferToImage(const VkBuffer& source, const VkImage& destination) const {
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
    }

    void VkCommandBuffer::transitionImage(const VkImage& image, const VkImageLayout layout) const {
        VkImageMemoryBarrier barrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_NONE,
            .dstAccessMask = VK_ACCESS_NONE,
            .oldLayout = image.getLayout(),
            .newLayout = layout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image.getImage(),
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = image.getMipLevels(),
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        VkPipelineStageFlags sourceFlags;
        VkPipelineStageFlags destinationFlags;

        if (image.getLayout() == VK_IMAGE_LAYOUT_UNDEFINED &&
            layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (image.getLayout() == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
            layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else {
            throw std::runtime_error("Unsupported transition layout");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceFlags,
            destinationFlags,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier
        );
    }

    void VkCommandBuffer::copyImage(const VkImage& source, const VkImage& destination) const {
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
    }
}
