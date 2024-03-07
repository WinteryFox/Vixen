#include "VkCommandBuffer.h"

#include "VkCommandPool.h"
#include "../IndexFormat.h"
#include "material/Material.h"

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

    void VkCommandBuffer::beginRenderPass(
        const uint32_t width,
        const uint32_t height,
        const uint8_t samples,
        const std::vector<AttachmentInfo>& attachments,
        const VkImageView& depthAttachment
    ) const {
        std::vector<VkRenderingAttachmentInfo> vkAttachments{attachments.size()};
        for (auto i = 0; i < attachments.size(); i++) {
            auto [loadAction, storeAction, layout, loadStoreTarget, resolveTarget, clearColor, clearDepth, clearStencil]
                = attachments[i];

            vkAttachments[i] = {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .pNext = nullptr,
                .imageView = loadStoreTarget,
                .imageLayout = layout,
                .resolveMode = storeAction == StoreAction::Resolve ||
                               storeAction == StoreAction::StoreAndResolve
                                   ? VK_RESOLVE_MODE_AVERAGE_BIT
                                   : VK_RESOLVE_MODE_NONE,
                .resolveImageView = resolveTarget,
                .resolveImageLayout = layout,
                .loadOp = toVkLoadAction(loadAction),
                .storeOp = toVkStoreAction(storeAction),
                .clearValue = {
                    .color = {
                        clearColor.r,
                        clearColor.g,
                        clearColor.b,
                        clearColor.a
                    },
                }
            };
        }

        VkRenderingAttachmentInfo depthAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = depthAttachment.getImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = nullptr,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {
                .depthStencil = {
                    1.0f,
                    0
                }
            }
        };

        const VkRenderingInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderArea = {
                .offset = {
                    .x = 0,
                    .y = 0
                },
                .extent = {
                    .width = width,
                    .height = height
                }
            },
            .layerCount = 1,
            .viewMask = 0,
            .colorAttachmentCount = static_cast<uint32_t>(vkAttachments.size()),
            .pColorAttachments = vkAttachments.data(),
            .pDepthAttachment = &depthAttachmentInfo,
            .pStencilAttachment = &depthAttachmentInfo
        };

        vkCmdBeginRendering(commandBuffer, &renderingInfo);
    }

    void VkCommandBuffer::endRenderPass() const {
        vkCmdEndRendering(commandBuffer);
    }

    void VkCommandBuffer::setViewport(const Rectangle rectangle) const {
        const VkViewport viewport{
            .x = rectangle.x,
            .y = rectangle.y,
            .width = rectangle.width,
            .height = rectangle.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    }

    void VkCommandBuffer::setScissor(const Rectangle rectangle) const {
        const VkRect2D rect{
            .offset = {
                .x = static_cast<int32_t>(rectangle.x),
                .y = static_cast<int32_t>(rectangle.y)
            },
            .extent = {
                .width = static_cast<uint32_t>(rectangle.width),
                .height = static_cast<uint32_t>(rectangle.height)
            }
        };

        vkCmdSetScissor(commandBuffer, 0, 1, &rect);
    }

    void VkCommandBuffer::drawMesh(const glm::mat4& modelMatrix, const VkMesh& mesh) const {
        const auto& vertexBuffer = mesh.getVertexBuffer().getBuffer();
        constexpr std::array<VkDeviceSize, 1> offsets{};
        vkCmdBindVertexBuffers(
            commandBuffer,
            0,
            1,
            &vertexBuffer,
            offsets.data()
        );

        vkCmdBindIndexBuffer(
            commandBuffer,
            mesh.getIndexBuffer().getBuffer(),
            0,
            mesh.getIndexFormat() == IndexFormat::UNSIGNED_INT_16
                ? VK_INDEX_TYPE_UINT16
                : VK_INDEX_TYPE_UINT32
        );

        mesh.getMaterial()->pipeline->bindGraphics(commandBuffer);

        const auto& set = mesh.getMaterial()->descriptorSet->getSet();
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            mesh.getMaterial()->pipeline->getLayout().getLayout(),
            0,
            1,
            &set,
            0,
            nullptr
        );

        vkCmdDrawIndexed(
            commandBuffer,
            mesh.getIndexCount(),
            1,
            0,
            0,
            0
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

    void VkCommandBuffer::transitionImage(VkImage& image, const VkImageLayout layout) const {
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
                .aspectMask = static_cast<VkImageAspectFlags>(
                    isDepthFormat(image.getFormat())
                        ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
                        : VK_IMAGE_ASPECT_COLOR_BIT
                ),
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
        } else if (image.getLayout() == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
            layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else if (layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            sourceFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            destinationFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        } else if (layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            barrier.dstAccessMask = 0;

            sourceFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            destinationFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        } else if (layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            sourceFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            destinationFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        } else
            throw std::runtime_error("Unsupported transition layout");

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

        // TODO: I hate this, but I'm not sure how else to solve this.
        image.layout = layout;
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
