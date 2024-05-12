#include "VkRenderer.h"

#include "Swapchain.h"

namespace Vixen::Vk {
    VkRenderer::VkRenderer(
        const std::shared_ptr<VkPipeline> &pipeline,
        const std::shared_ptr<Swapchain> &swapchain
    ) : device(pipeline->getDevice()),
        swapchain(swapchain),
        pipelineLayout(std::make_unique<VkPipelineLayout>(device, pipeline->getProgram())),
        pipeline(pipeline) {}

    VkRenderer::~VkRenderer() {
        device->waitIdle();
    }

    void VkRenderer::render(const std::vector<VkMesh> &meshes) {
        if (const auto state = swapchain->acquireImage(
            std::numeric_limits<uint64_t>::max(),
            [this, &meshes](const FrameData &frame) {
                prepare(frame, meshes);

                const std::vector signalSemaphores = {frame.renderFinishedSemaphore.getSemaphore()};

                frame.commandBuffer.submit(
                    device->getGraphicsQueue(),
                    {frame.imageAvailableSemaphore.getSemaphore()},
                    {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
                    signalSemaphores
                );

                swapchain->present(signalSemaphores);

                cleanup();
            }
        ); state == Swapchain::State::OutOfDate) {
            device->waitIdle();

            swapchain->invalidate();
        }
    }

    void VkRenderer::prepare(
        const FrameData &frame,
        const std::vector<VkMesh> &meshes
    ) const {
        const auto &[width, height] = swapchain->getExtent();
        const auto &commandBuffer = frame.commandBuffer;

        commandBuffer.reset();
        commandBuffer.begin(CommandBufferUsage::Simultanious);

        commandBuffer.beginRenderPass(
            width,
            height,
            1,
            {
                {
                    .loadAction = LoadAction::Clear,
                    .storeAction = StoreAction::Store,
                    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .loadStoreTarget = frame.colorImageView->getImageView(),
                    .resolveTarget = nullptr,
                    .clearColor = {0.0F, 0.0F, 0.0F, 0.0F},
                    .clearDepth = 0.0F,
                    .clearStencil = 0
                }
            },
            *frame.depthImageView
        );

        const Rectangle rectangle{
            .x = 0,
            .y = 0,
            .width = static_cast<float>(width),
            .height = static_cast<float>(height)
        };
        commandBuffer.setViewport(rectangle);
        commandBuffer.setScissor(rectangle);

        for (const auto &mesh: meshes) {
            commandBuffer.drawMesh(glm::mat4(1.0), mesh);
        }

        commandBuffer.endRenderPass();

        commandBuffer.end();
    }

    void VkRenderer::cleanup() {
        deletionQueue.flush();
    }
}
