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

        for (size_t i = 0; i < swapchain.getImageCount(); i++) {
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
        }

        framebuffers.reserve(swapchain.getImageCount());
        for (size_t i = 0; i < swapchain.getImageCount(); i++) {
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
        }
    }

    VkRenderer::~VkRenderer() {

    }

    void VkRenderer::render() {
        auto &commandBuffer = renderCommandBuffers[0];
        auto c = commandBuffer.getCommandBuffer();

        prepare(commandBuffer);

        auto graphicsQueue = device->getGraphicsQueue();
        bool invalid = swapchain.acquireImage([this, &c, &graphicsQueue](const auto &imageIndex, auto &fence) {
            VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

            VkSubmitInfo info{
                    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                    .waitSemaphoreCount = 0,
                    // TODO
                    .pWaitSemaphores = nullptr,
                    .pWaitDstStageMask = waitStages,
                    .commandBufferCount = 1,
                    .pCommandBuffers = &c,
                    .signalSemaphoreCount = 0,
                    .pSignalSemaphores = nullptr
            };

            checkVulkanResult(
                    vkQueueSubmit(graphicsQueue, 1, &info, fence),
                    "Failed to submit to queue"
            );

            present(imageIndex);
        });

        if (invalid)
            spdlog::warn("Outdated swapchain state");
    }

    void VkRenderer::present(uint32_t imageIndex) {
        const auto &s = swapchain.getSwapchain();
        VkPresentInfoKHR info{
                .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .waitSemaphoreCount = 0,
                .pWaitSemaphores = nullptr,
                .swapchainCount = 1,
                .pSwapchains = &s,
                .pImageIndices = &imageIndex,
                .pResults = nullptr,
        };

        vkQueuePresentKHR(device->getPresentQueue(), &info);
    }

    void VkRenderer::prepare(VkCommandBuffer &commandBuffer) {
        commandBuffer.record([this](::VkCommandBuffer commandBuffer) {
            std::vector<VkClearValue> clearValues{
                    {
                            .color = {
                                    0.0f,
                                    0.0f,
                                    1.0f,
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
                    .framebuffer = framebuffers[0].getFramebuffer(),
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

            pipeline->bind(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
            vkCmdDraw(commandBuffer, 3, 1, 0, 0);

            vkCmdEndRenderPass(commandBuffer);
        }, VkCommandBuffer::Usage::SIMULTANEOUS);
    }
}
