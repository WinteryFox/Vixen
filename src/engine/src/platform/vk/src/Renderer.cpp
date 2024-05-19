#include "Renderer.h"

#include "AttachmentInfo.h"
#include "Rectangle.h"
#include "VulkanSwapchain.h"
#include "commandbuffer/CommandBufferUsage.h"

namespace Vixen {
    Renderer::Renderer(
        const std::shared_ptr<VulkanPipeline> &pipeline,
        const std::shared_ptr<VulkanSwapchain> &swapchain
    ) : device(pipeline->getDevice()),
        swapchain(swapchain),
        pipelineLayout(std::make_unique<VulkanPipelineLayout>(device, pipeline->getProgram())),
        pipeline(pipeline) {}

    Renderer::~Renderer() {
        device->waitIdle();
    }

    void Renderer::render(const std::vector<VulkanMesh> &meshes) {
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
        ); state == VulkanSwapchain::State::OutOfDate) {
            device->waitIdle();

            swapchain->invalidate();
        }
    }

    void Renderer::prepare(
        const FrameData &frame,
        const std::vector<VulkanMesh> &meshes
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
                    .loadStoreTarget = frame.colorImageView,
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

    void Renderer::cleanup() {}
}
