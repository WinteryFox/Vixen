#include "VkRenderer.h"

#include "Swapchain.h"

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

                prepare(imageIndex, commandBuffer, mesh, descriptorSets);

                std::vector<::VkSemaphore> signalSemaphores = {renderFinishedSemaphores[currentFrame].getSemaphore()};

                commandBuffer.submit(
                    device->getGraphicsQueue(),
                    {imageAvailableSemaphore.getSemaphore()},
                    {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
                    signalSemaphores
                );

                swapchain->present(imageIndex, signalSemaphores);
            }
        ); state == Swapchain::State::OUT_OF_DATE) {
            device->waitIdle();

            swapchain->invalidate();
        }
    }

    void VkRenderer::prepare(
        const uint32_t imageIndex,
        const VkCommandBuffer& commandBuffer,
        const VkMesh& mesh,
        const std::vector<::VkDescriptorSet>& descriptorSets
    ) const {
        const auto& [width, height] = swapchain->getExtent();

        commandBuffer.wait();
        commandBuffer.reset();

        commandBuffer.begin(CommandBufferUsage::SIMULTANEOUS);

        commandBuffer.transitionImage(
            *swapchain->getColorImages()[imageIndex],
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        );
        commandBuffer.transitionImage(
            *swapchain->getDepthImages()[imageIndex],
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        );

        commandBuffer.beginRenderPass(
            width,
            height,
            1,
            {
                {
                    .loadAction = LoadAction::Clear,
                    .storeAction = StoreAction::Store,
                    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .loadStoreTarget = swapchain->getColorImageViews()[imageIndex].getImageView(),
                    .resolveTarget = nullptr,
                    .clearColor = {0.0f, 0.0f, 0.0f, 0.0f},
                    .clearDepth = 0.0f,
                    .clearStencil = 0
                }
            },
            swapchain->getDepthImageViews()[imageIndex].getImageView()
        );

        // TODO: Make material system, which will automatically bind the pipeline and descriptor sets
        commandBuffer.record([&descriptorSets, this](const auto& cmd) {
            pipeline->bindGraphics(cmd);

            vkCmdBindDescriptorSets(
                cmd,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout->getLayout(),
                0,
                descriptorSets.size(),
                descriptorSets.data(),
                0,
                nullptr
            );
        });

        const Rectangle& rectangle{
            .x = 0,
            .y = 0,
            .width = static_cast<float>(width),
            .height = static_cast<float>(height)
        };
        commandBuffer.setViewport(rectangle);
        commandBuffer.setScissor(rectangle);

        commandBuffer.drawMesh(mesh);

        commandBuffer.endRenderPass();

        commandBuffer.transitionImage(
            *swapchain->getColorImages()[imageIndex],
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        );

        commandBuffer.end();
    }
}
