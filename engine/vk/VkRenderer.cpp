#include "VkRenderer.h"

namespace Vixen::Vk {
    VkRenderer::VkRenderer(
            const std::shared_ptr<Vk::Device> &device,
            Swapchain &swapchain,
            const std::shared_ptr<Vk::VkPipeline> &pipeline
    ) : device(device),
        swapchain(swapchain),
        pipeline(pipeline),
        pipelineLayout(std::make_unique<VkPipelineLayout>(device, pipeline->getProgram())),
        renderCommandPool(std::make_shared<VkCommandPool>(device, VkCommandPool::Usage::GRAPHICS, true)),
        renderCommandBuffers(
                VkCommandBuffer::allocateCommandBuffers(
                        renderCommandPool,
                        VkCommandBuffer::Level::PRIMARY,
                        swapchain.getImageCount()
                )
        ) {
        const auto &renderPass = pipeline->getRenderPass();

        const auto imageCount = swapchain.getImageCount();
        depthImages.reserve(imageCount);
        depthImageViews.reserve(imageCount);
        framebuffers.reserve(imageCount);
        renderFinishedSemaphores.reserve(imageCount);
        for (size_t i = 0; i < imageCount; i++) {
            depthImages.emplace_back(
                    device,
                    swapchain.getExtent().width,
                    swapchain.getExtent().height,
                    VK_FORMAT_D32_SFLOAT,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
            );

            depthImageViews.emplace_back(
                    std::make_unique<VkImageView>(
                            depthImages[i],
                            VK_IMAGE_ASPECT_DEPTH_BIT
                    )
            );

            framebuffers.emplace_back(
                    device,
                    renderPass,
                    swapchain.getExtent().width,
                    swapchain.getExtent().height,
                    std::vector<::VkImageView>{
                            swapchain.getImageViews()[i],
                            depthImageViews[i]->getImageView()
                    }
            );

            renderFinishedSemaphores.emplace_back(device);
        }
    }

    VkRenderer::~VkRenderer() {
        vkDeviceWaitIdle(device->getDevice());
    }

    void VkRenderer::render() {
        auto graphicsQueue = device->getGraphicsQueue();
        auto isSwapchainOutdated = swapchain.acquireImage([this, &graphicsQueue](
                const auto &imageIndex,
                const auto &semaphore,
                const auto &fence
        ) {
            auto waitSemaphore = semaphore.getSemaphore();
            auto signalSemaphore = renderFinishedSemaphores[imageIndex].getSemaphore();

            auto &commandBuffer = renderCommandBuffers[imageIndex];
            auto c = commandBuffer.getCommandBuffer();

            prepare(commandBuffer, framebuffers[imageIndex]);

            VkPipelineStageFlags waitStages[] = {
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            };

            VkSubmitInfo submitInfo{
                    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                    .waitSemaphoreCount = 1,
                    .pWaitSemaphores = &waitSemaphore,
                    .pWaitDstStageMask = waitStages,
                    .commandBufferCount = 1,
                    .pCommandBuffers = &c,
                    .signalSemaphoreCount = 1,
                    .pSignalSemaphores = &signalSemaphore
            };

            checkVulkanResult(
                    vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence),
                    "Failed to submit to queue"
            );

            const auto &s = swapchain.getSwapchain();
            VkPresentInfoKHR presentInfo{
                    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                    .waitSemaphoreCount = 1,
                    .pWaitSemaphores = &signalSemaphore,
                    .swapchainCount = 1,
                    .pSwapchains = &s,
                    .pImageIndices = &imageIndex,
                    .pResults = nullptr,
            };

            vkQueuePresentKHR(device->getPresentQueue(), &presentInfo);
        }, std::numeric_limits<uint64_t>::max());

        if (isSwapchainOutdated)
            spdlog::warn("Swapchain is outdated");
            //swapchain = Swapchain();
    }

    void VkRenderer::prepare(VkCommandBuffer &commandBuffer, VkFramebuffer &framebuffer) {
        commandBuffer.reset();
        commandBuffer.record([this, &framebuffer](::VkCommandBuffer commandBuffer) {
            std::vector<VkClearValue> clearValues{
                    {
                            .color = {
                                    0.0f,
                                    0.0f,
                                    0.0f,
                                    1.0f
                            }
                    },
                    {
                            .depthStencil = {
                                    1.0f,
                                    0
                            }
                    }
            };

            VkRenderPassBeginInfo renderPassInfo{
                    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                    .renderPass = pipeline->getRenderPass().getRenderPass(),
                    .framebuffer = framebuffer.getFramebuffer(),
                    .renderArea = {
                            .offset = {
                                    .x = 0,
                                    .y = 0
                            },
                            .extent = swapchain.getExtent(),
                    },
                    .clearValueCount = static_cast<uint32_t>(clearValues.size()),
                    .pClearValues = clearValues.data()
            };

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            pipeline->bindGraphics(commandBuffer);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(swapchain.getExtent().width);
            viewport.height = static_cast<float>(swapchain.getExtent().height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = swapchain.getExtent();
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            vkCmdDraw(commandBuffer, 3, 1, 0, 0);

            vkCmdEndRenderPass(commandBuffer);
        }, VkCommandBuffer::Usage::SIMULTANEOUS);
    }
}
