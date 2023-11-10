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
        renderCommandPool(std::make_shared<VkCommandPool>(
                device->getDevice(),
                device->getGraphicsQueueFamily().index,
                VkCommandPool::Usage::GRAPHICS,
                true
        )),
        renderCommandBuffers(
                renderCommandPool->allocateCommandBuffers(
                        VkCommandBuffer::Level::PRIMARY,
                        swapchain.getImageCount()
                )
        ) {
        const auto imageCount = swapchain.getImageCount();
        renderFinishedSemaphores.reserve(imageCount);
        for (size_t i = 0; i < imageCount; i++)
            renderFinishedSemaphores.emplace_back(device);

        createFramebuffers();
    }

    VkRenderer::~VkRenderer() {
        device->waitIdle();
    }

    void VkRenderer::render(const VkBuffer &buffer, uint32_t vertexCount, uint32_t indexCount) {
        auto state = swapchain.acquireImage(
                std::numeric_limits<uint64_t>::max(),
                [this, &buffer, &vertexCount, &indexCount](
                        const auto &currentFrame,
                        const auto &imageIndex,
                        const auto &imageAvailableSemaphore
                ) {
                    auto &commandBuffer = renderCommandBuffers[currentFrame];

                    prepare(commandBuffer, framebuffers[imageIndex], buffer, vertexCount, indexCount);

                    std::vector<::VkSemaphore> waitSemaphores = {imageAvailableSemaphore.getSemaphore()};
                    std::vector<::VkSemaphore> signalSemaphores = {
                            renderFinishedSemaphores[currentFrame].getSemaphore()};

                    commandBuffer.submit(
                            device->getGraphicsQueue(),
                            waitSemaphores,
                            {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
                            signalSemaphores
                    );

                    swapchain.present(imageIndex, signalSemaphores);
                }
        );

        if (state == Swapchain::State::OUT_OF_DATE) {
            device->waitIdle();

            framebuffers.clear();
            depthImageViews.clear();
            depthImages.clear();

            swapchain.invalidate();

            createFramebuffers();
        }
    }

    void VkRenderer::prepare(
            VkCommandBuffer &commandBuffer,
            VkFramebuffer &framebuffer,
            const VkBuffer &buffer,
            uint32_t vertexCount,
            uint32_t indexCount
    ) {
        commandBuffer.record(
                VkCommandBuffer::Usage::SIMULTANEOUS,
                [this, &framebuffer, &buffer, &vertexCount, &indexCount](::VkCommandBuffer commandBuffer) {
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

                    VkViewport viewport{
                            .x = 0.0f,
                            .y = 0.0f,
                            .width = static_cast<float>(swapchain.getExtent().width),
                            .height = static_cast<float>(swapchain.getExtent().height),
                            .minDepth = 0.0f,
                            .maxDepth = 1.0f,
                    };
                    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

                    VkRect2D scissor{
                            .offset = {0, 0},
                            .extent = swapchain.getExtent(),
                    };
                    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

                    ::VkBuffer vertexBuffers[1]{buffer.getBuffer()};
                    // TODO: Hardcoded buffer offset should probably automatically be determined somehow
                    ::VkDeviceSize offsets[] = {0};

                    vkCmdBindVertexBuffers(
                            commandBuffer,
                            0,
                            1,
                            vertexBuffers,
                            offsets
                    );

                    vkCmdBindIndexBuffer(
                            commandBuffer,
                            buffer.getBuffer(),
                            vertexCount * 24,
                            VK_INDEX_TYPE_UINT32
                    );

                    // TODO: Hardcoded buffer offset should probably automatically be determined somehow
                    vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);

                    vkCmdEndRenderPass(commandBuffer);
                }
        );
    }

    void VkRenderer::createFramebuffers() {
        const auto &imageCount = swapchain.getImageCount();

        depthImages.reserve(imageCount);
        depthImageViews.reserve(imageCount);
        framebuffers.reserve(imageCount);
        for (size_t i = 0; i < imageCount; i++) {
            depthImages.emplace_back(
                    device,
                    swapchain.getExtent().width,
                    swapchain.getExtent().height,
                    VK_SAMPLE_COUNT_1_BIT,
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
                    pipeline->getRenderPass(),
                    swapchain.getExtent().width,
                    swapchain.getExtent().height,
                    std::vector<::VkImageView>{
                            swapchain.getImageViews()[i],
                            depthImageViews[i]->getImageView()
                    }
            );
        }
    }
}