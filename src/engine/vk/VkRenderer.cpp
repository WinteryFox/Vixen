#include "VkRenderer.h"

#include "../IndexFormat.h"

namespace Vixen::Vk {
    VkRenderer::VkRenderer(
        const std::shared_ptr<VkPipeline>& pipeline,
        const std::shared_ptr<Swapchain>& swapchain
    ) : device(pipeline->getDevice()),
        swapchain(swapchain),
        pipelineLayout(std::make_unique<VkPipelineLayout>(device, pipeline->getProgram())),
        pipeline(pipeline),
        renderCommandPool(std::make_shared<VkCommandPool>(
            device,
            device->getGraphicsQueueFamily().index,
            CommandPoolUsage::GRAPHICS,
            true
        )),
        renderCommandBuffers(
            renderCommandPool->allocate(
                CommandBufferLevel::PRIMARY,
                swapchain->getImageCount()
            )
        ) {
        const auto imageCount = swapchain->getImageCount();
        renderFinishedSemaphores.reserve(imageCount);
        for (size_t i = 0; i < imageCount; i++)
            renderFinishedSemaphores.emplace_back(device);

        createFramebuffers();
    }

    VkRenderer::~VkRenderer() {
        device->waitIdle();
    }

    void VkRenderer::render(
        const VkMesh& mesh,
        const std::vector<::VkDescriptorSet>& descriptorSets
    ) {
        if (const auto state = swapchain->acquireImage(
            std::numeric_limits<uint64_t>::max(),
            [this, &mesh, &descriptorSets](
            const auto& currentFrame,
            const auto& imageIndex,
            const auto& imageAvailableSemaphore
        ) {
                auto& commandBuffer = renderCommandBuffers[currentFrame];

                prepare(commandBuffer, framebuffers[imageIndex], mesh, descriptorSets);

                std::vector<::VkSemaphore> signalSemaphores = {renderFinishedSemaphores[currentFrame].getSemaphore()};

                commandBuffer.submit(
                    device->getGraphicsQueue(),
                    {imageAvailableSemaphore.getSemaphore()},
                    {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
                    {renderFinishedSemaphores[currentFrame].getSemaphore()}
                );

                swapchain->present(imageIndex, signalSemaphores);
            }
        ); state == Swapchain::State::OUT_OF_DATE) {
            device->waitIdle();

            framebuffers.clear();
            depthImageViews.clear();
            depthImages.clear();

            swapchain->invalidate();

            createFramebuffers();
        }
    }

    void VkRenderer::prepare(
        VkCommandBuffer& commandBuffer,
        VkFramebuffer& framebuffer,
        const VkMesh& mesh,
        const std::vector<::VkDescriptorSet>& descriptorSets
    ) const {
        commandBuffer.record(
            CommandBufferUsage::SIMULTANEOUS,
            [this, &framebuffer, &mesh, &descriptorSets](::VkCommandBuffer commandBuffer) {
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

                const VkRenderPassBeginInfo renderPassInfo{
                    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                    .renderPass = pipeline->getRenderPass().getRenderPass(),
                    .framebuffer = framebuffer.getFramebuffer(),
                    .renderArea = {
                        .offset = {
                            .x = 0,
                            .y = 0
                        },
                        .extent = swapchain->getExtent(),
                    },
                    .clearValueCount = static_cast<uint32_t>(clearValues.size()),
                    .pClearValues = clearValues.data()
                };

                vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

                pipeline->bindGraphics(commandBuffer);

                const VkViewport viewport{
                    .x = 0.0f,
                    .y = 0.0f,
                    .width = static_cast<float>(swapchain->getExtent().width),
                    .height = static_cast<float>(swapchain->getExtent().height),
                    .minDepth = 0.0f,
                    .maxDepth = 1.0f,
                };
                vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

                const VkRect2D scissor{
                    .offset = {0, 0},
                    .extent = swapchain->getExtent(),
                };
                vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

                const ::VkBuffer vertexBuffers[1]{mesh.getVertexBuffer().getBuffer()};
                // TODO: Hardcoded buffer offset should probably automatically be determined somehow
                constexpr VkDeviceSize offsets[] = {0};

                vkCmdBindVertexBuffers(
                    commandBuffer,
                    0,
                    1,
                    vertexBuffers,
                    offsets
                );

                vkCmdBindIndexBuffer(
                    commandBuffer,
                    mesh.getIndexBuffer().getBuffer(),
                    // TODO: Remove hardcoded offset
                    0,
                    mesh.getIndexFormat() == IndexFormat::UNSIGNED_INT_16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32
                );

                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout->getLayout(),
                    0,
                    descriptorSets.size(),
                    descriptorSets.data(),
                    0,
                    nullptr
                );

                // TODO: Hardcoded buffer offset should probably automatically be determined somehow
                vkCmdDrawIndexed(commandBuffer, mesh.getIndexCount(), 1, 0, 0, 0);

                vkCmdEndRenderPass(commandBuffer);
            }
        );
    }

    void VkRenderer::createFramebuffers() {
        const auto& imageCount = swapchain->getImageCount();

        depthImages.reserve(imageCount);
        depthImageViews.reserve(imageCount);
        framebuffers.reserve(imageCount);
        for (size_t i = 0; i < imageCount; i++) {
            depthImages.push_back(
                std::make_shared<VkImage>(
                    device,
                    swapchain->getExtent().width,
                    swapchain->getExtent().height,
                    VK_SAMPLE_COUNT_1_BIT,
                    VK_FORMAT_D32_SFLOAT,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    1
                )
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
                swapchain->getExtent().width,
                swapchain->getExtent().height,
                std::vector{
                    swapchain->getImageViews()[i],
                    depthImageViews[i]->getImageView()
                }
            );
        }
    }
}
