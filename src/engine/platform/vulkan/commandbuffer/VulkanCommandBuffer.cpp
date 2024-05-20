#include "VulkanCommandBuffer.h"

#include "CommandBufferUsage.h"
#include "VulkanCommandPool.h"
#include "buffer/VulkanBuffer.h"
#include "core/AttachmentInfo.h"
#include "core/IndexFormat.h"
#include "core/Rectangle.h"
#include "descriptorset/VulkanDescriptorSet.h"
#include "device/VulkanDevice.h"
#include "image/VulkanImageView.h"
#include "material/Material.h"
#include "pipeline/VulkanPipeline.h"
#include "rendering/VulkanMesh.h"

namespace Vixen {
    VulkanCommandBuffer::VulkanCommandBuffer(
        const std::shared_ptr<VulkanCommandPool> &commandPool,
        const VkCommandBuffer commandBuffer
    ) : commandPool(commandPool),
        commandBuffer(commandBuffer),
        fence(commandPool->getDevice(), true) {}

    VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandBuffer &&other) noexcept
        : commandPool(std::exchange(other.commandPool, nullptr)),
          commandBuffer(std::exchange(other.commandBuffer, nullptr)),
          fence(std::move(other.fence)) {}

    VulkanCommandBuffer &VulkanCommandBuffer::operator=(VulkanCommandBuffer &&other) noexcept {
        std::swap(commandPool, other.commandPool);
        std::swap(commandBuffer, other.commandBuffer);
        std::swap(fence, other.fence);

        return *this;
    }

    VulkanCommandBuffer::~VulkanCommandBuffer() {
        if (commandPool == nullptr)
            return;

        fence.wait();
        vkFreeCommandBuffers(
            commandPool->getDevice()->getDevice(),
            commandPool->getCommandPool(),
            1,
            &commandBuffer
        );
    }

    void VulkanCommandBuffer::wait() const {
        fence.wait();
    }

    void VulkanCommandBuffer::reset() const {
        wait();
        checkVulkanResult(
            vkResetCommandBuffer(commandBuffer, 0),
            "Failed to reset command buffer"
        );
    }

    void VulkanCommandBuffer::begin(const CommandBufferUsage usage) const {
        VkCommandBufferBeginInfo info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pInheritanceInfo = nullptr
        };

        switch (usage) {
            case CommandBufferUsage::Once:
                info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                break;
            case CommandBufferUsage::Simultanious:
                info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
                break;
        }

        checkVulkanResult(
            vkBeginCommandBuffer(commandBuffer, &info),
            "Failed to begin command buffer"
        );
    }

    void VulkanCommandBuffer::end() const {
        checkVulkanResult(
            vkEndCommandBuffer(commandBuffer),
            "Failed to end command buffer"
        );
    }

    void VulkanCommandBuffer::submit(
        ::VkQueue queue,
        const std::vector<::VkSemaphore> &waitSemaphores,
        const std::vector<::VkPipelineStageFlags> &waitMasks,
        const std::vector<::VkSemaphore> &signalSemaphores
    ) const {
        fence.wait();
        fence.reset();

        const VkSubmitInfo info{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,

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

    void VulkanCommandBuffer::beginRenderPass(
        const uint32_t width,
        const uint32_t height,
        // TODO: Currently does nothing and the samples is dependent on the swapchain
        const uint8_t samples,
        const std::vector<AttachmentInfo> &attachments,
        const VulkanImageView &depthAttachment
    ) const {
        std::vector<VkRenderingAttachmentInfo> vkAttachments;
        vkAttachments.reserve(attachments.size());
        for (auto [loadAction, storeAction, loadStoreTarget, resolveTarget, clearColor, clearDepth, clearStencil]:
             attachments)
            vkAttachments.push_back({
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .pNext = nullptr,
                .imageView = !loadStoreTarget ? nullptr : loadStoreTarget,
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .resolveMode = storeAction == StoreAction::Resolve ||
                               storeAction == StoreAction::StoreAndResolve
                                   ? VK_RESOLVE_MODE_AVERAGE_BIT
                                   : VK_RESOLVE_MODE_NONE,
                .resolveImageView = !resolveTarget ? nullptr : resolveTarget,
                .resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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
            });

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

    void VulkanCommandBuffer::endRenderPass() const {
        vkCmdEndRendering(commandBuffer);
    }

    void VulkanCommandBuffer::setViewport(const Rectangle &rectangle) const {
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

    void VulkanCommandBuffer::setScissor(const Rectangle &rectangle) const {
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

    void VulkanCommandBuffer::drawMesh(const glm::mat4 &modelMatrix, const VulkanMesh &mesh) const {
        const auto &vertexBuffer = mesh.getVertexBuffer().getBuffer();
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
            mesh.getIndexFormat() == IndexFormat::UnsignedInt16
                ? VK_INDEX_TYPE_UINT16
                : VK_INDEX_TYPE_UINT32
        );

        mesh.getMaterial()->pipeline->bindGraphics(commandBuffer);

        const auto &set = mesh.getMaterial()->descriptorSet->getSet();
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

    void VulkanCommandBuffer::copyBuffer(const VulkanBuffer &source, const VulkanBuffer &destination) const {
        const VkBufferCopy region{
            .srcOffset = 0,
            .dstOffset = 0,
            .size = source.getSize(),
        };

        vkCmdCopyBuffer(commandBuffer, source.getBuffer(), destination.getBuffer(), 1, &region);
    }

    void VulkanCommandBuffer::copyBufferToImage(const VulkanBuffer &source, const VulkanImage &destination) const {
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

        vkCmdCopyBufferToImage(commandBuffer, source.getBuffer(), destination.getImage(),
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    void VulkanCommandBuffer::transitionImage(
        VulkanImage &image,
        const VkImageLayout oldLayout,
        const VkImageLayout newLayout,
        const uint32_t baseMipLevel,
        const uint32_t mipLevels
    ) const {
        VkImageMemoryBarrier barrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_NONE,
            .dstAccessMask = VK_ACCESS_NONE,
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image.getImage(),
            .subresourceRange = {
                .aspectMask = static_cast<VkImageAspectFlags>(
                    isDepthFormat(image.getFormat())
                        ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
                        : VK_IMAGE_ASPECT_COLOR_BIT
                ),
                .baseMipLevel = baseMipLevel,
                .levelCount = mipLevels,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        VkPipelineStageFlags sourceFlags;
        VkPipelineStageFlags destinationFlags;

        switch (oldLayout) {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                barrier.srcAccessMask = VK_ACCESS_NONE;
                sourceFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                sourceFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                sourceFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                sourceFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                break;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                sourceFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                break;
            default:
                spdlog::error("Unsupported source layout {} for image transition",
                              string_VkImageLayout(oldLayout));
                throw std::runtime_error("Unsupported source layout for image transition");
        }

        switch (newLayout) {
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                destinationFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                destinationFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                destinationFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                break;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            // TODO: Dodgy flag, do we only read images in the fragment shader? Probably not.
                destinationFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                break;
            default:
                spdlog::error("Unsupported destination layout {} for image transition",
                              string_VkImageLayout(newLayout));
                throw std::runtime_error("Unsupported destination layout for image transition");
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

    void VulkanCommandBuffer::blitImage(const VulkanImage &source, VulkanImage &destination) const {
        if (!(commandPool->getDevice()->getGpu().getFormatProperties(source.getFormat()).optimalTilingFeatures &
              VK_FORMAT_FEATURE_BLIT_SRC_BIT))
            throw std::runtime_error("Source image format does not support blitting");
        if (!(commandPool->getDevice()->getGpu().getFormatProperties(destination.getFormat()).optimalTilingFeatures &
              VK_FORMAT_FEATURE_BLIT_DST_BIT))
            throw std::runtime_error("Destination image format does not support blitting");

        for (uint32_t i = 1; i < destination.getMipLevels(); i++) {
            transitionImage(destination, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, i,
                            1);

            const VkImageBlit region{
                .srcSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = i - 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                },
                .srcOffsets = {
                    {
                        .x = 0,
                        .y = 0,
                        .z = 0
                    },
                    {
                        .x = static_cast<int32_t>(source.getWidth() >> (i - 1)),
                        .y = static_cast<int32_t>(source.getHeight() >> (i - 1)),
                        .z = 1
                    }
                },
                .dstSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = i,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                },
                .dstOffsets = {
                    {
                        .x = 0,
                        .y = 0,
                        .z = 0
                    },
                    {
                        .x = static_cast<int32_t>(destination.getWidth() >> i),
                        .y = static_cast<int32_t>(destination.getHeight() >> i),
                        .z = 1
                    }
                }
            };

            vkCmdBlitImage(
                commandBuffer,
                source.getImage(),
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                destination.getImage(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region,
                VK_FILTER_LINEAR
            );

            transitionImage(destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, i,
                            1);
        }
    }

    // TODO: Not sure how to handle the different regions with mip-maps
    void VulkanCommandBuffer::copyImage(const VulkanImage &source, const VulkanImage &destination) const {
        if (source.getSampleCount() != destination.getSampleCount())
            throw std::runtime_error("Source image sample count must match destination image sample count");

        std::vector<VkImageCopy> regions{source.getMipLevels()};

        for (uint32_t i = 0; i < regions.size(); i++) {
            regions[i] = {
                .srcSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = i,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
                .srcOffset = {
                    .x = 0,
                    .y = 0,
                    .z = 0
                },
                .dstSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = i,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
                .dstOffset = {
                    .x = 0,
                    .y = 0,
                    .z = 0
                },
                .extent = {
                    .width = source.getWidth(),
                    .height = source.getHeight(),
                    .depth = 1
                }
            };
        }

        vkCmdCopyImage(
            commandBuffer,
            source.getImage(),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            destination.getImage(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            regions.size(),
            regions.data()
        );
    }
}
