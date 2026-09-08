#include <experimental/scope>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>

#include "core/display/DisplayServer.h"
#include "core/framegraph/FrameGraph.h"
#include "core/framegraph/FrameGraphError.h"
#include "core/framegraph/RenderPassContext.h"
#include "core/image/Image.h"
#include "core/pipeline/GraphicsPipeline.h"
#include "core/pipeline/GraphicsPipelineDescription.h"
#include "core/rendering/Framebuffer.h"
#include "core/rendering/RenderingDevice.h"
#include "core/rendering/RenderingDeviceDriver.h"
#include "core/shader/ShaderLanguage.h"
#include "spdlog/spdlog.h"

namespace {
    using namespace Vixen;

    template <typename T, typename E>
    T require(std::expected<T, E> result) {
        if (!result)
            throw std::runtime_error(result.error().message);
        if constexpr (!std::is_void_v<T>)
            return std::move(*result);
    }

    std::string readShader(const char* filename) {
        const auto path = std::string{VIXEN_EDITOR_SHADER_DIR} + '/' + filename;
        std::ifstream stream(path);
        if (!stream)
            throw std::runtime_error("Cannot open triangle shader: " + path);

        return {std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
    }

    struct TrianglePipeline {
        RenderingDevice& device;
        const ImageDataFormat format;
        const ImageSamples samples;
        PipelineLayout* layout;
        GraphicsPipeline* pipeline;

        TrianglePipeline(
            RenderingDevice& device,
            const Image& target
        ) : device(device),
            format(target.view.format),
            samples(target.format.samples) {
            auto& driver = *device.getRenderingDeviceDriver();
            const auto shader = driver.createShaderFromSpirv(
                "Triangle",
                {
                    {
                        .stage = ShaderStageBits::Vertex,
                        .spirv = driver.compileSpirvFromSource(
                            ShaderStageBits::Vertex,
                            readShader("pbr.vertex.glsl"),
                            ShaderLanguage::GLSL
                        )
                    },
                    {
                        .stage = ShaderStageBits::Fragment,
                        .spirv = driver.compileSpirvFromSource(
                            ShaderStageBits::Fragment,
                            readShader("pbr.fragment.glsl"),
                            ShaderLanguage::GLSL
                        )
                    }
                }
            );

            auto shaderCleanup = std::experimental::scope_exit([&] { driver.destroyShader(shader); });

            layout = require(device.createPipelineLayout({}));
            auto layoutCleanup = std::experimental::scope_exit([&] { driver.destroyPipelineLayout(layout); });

            GraphicsPipelineDescription description{
                .shader = shader,
                .layout = layout,
                .state = {}
            };
            description.state.colorFormats = {format};
            description.state.colorBlending.resize(1);
            description.state.multisampling.samples = samples;
            pipeline = require(device.createGraphicsPipeline(description));
            layoutCleanup.release();
        }

        TrianglePipeline(const TrianglePipeline&) = delete;
        TrianglePipeline& operator=(const TrianglePipeline&) = delete;

        ~TrianglePipeline() {
            device.deferRelease([pipeline = pipeline, layout = layout](RenderingDeviceDriver& driver) {
                driver.destroyPipeline(pipeline);
                driver.destroyPipelineLayout(layout);
            });
        }
    };
}

int main() {
    try {
        spdlog::set_level(spdlog::level::trace);
        DisplayServer display(
            "Vixen " ENGINE_VERSION,
            {ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR, ENGINE_VERSION_PATCH},
            RenderingDriver::Vulkan,
            WindowMode::Windowed,
            VSyncMode::Enabled,
            WindowBits::Resizable,
            {1280, 720}
        );

        std::unique_ptr<TrianglePipeline> triangle;
        const auto window = display.getMainWindow();

        const auto draw = [&](RenderingDevice& device, const Framebuffer& framebuffer) {
            auto& target = *framebuffer.colorTarget;
            if (!triangle ||
                triangle->format != target.view.format ||
                triangle->samples != target.format.samples)
                triangle = std::make_unique<TrianglePipeline>(device, target);

            FrameGraph::Builder builder;
            const auto color = builder.importImage(
                "Screen color",
                target,
                {
                    .stages = PipelineStageBits::Copy,
                    .layout = ImageLayout::Undefined
                },
                {
                    .stages = PipelineStageBits::ColorAttachmentOutput,
                    .access = BarrierAccessBits::ColorAttachmentWrite,
                    .layout = ImageLayout::ColorAttachmentOptimal
                }
            );

            struct PassData {
                ImageHandle color;
            };

            builder.addGraphicsPass<PassData>(
                "Triangle",
                [&](RenderPass::Builder& pass, PassData& data) {
                    data.color = pass.addColorAttachment(
                        color,
                        LoadAction::Clear,
                        StoreAction::Store,
                        {
                            .color = {
                                0.025f,
                                0.025f,
                                0.04f,
                                1.0f
                            },
                            .depth = 1.0f,
                            .stencil = 0
                        }
                    );
                },
                [pipeline = triangle->pipeline](
                    const PassData& data,
                    const RenderPassContext& context
                ) -> RenderPass::RenderPassCallbackResult {
                    auto image = context.resources.writeImage(
                        data.color,
                        ImageUsageBits::ColorAttachment
                    );
                    if (!image)
                        return std::unexpected{
                            RenderPass::RenderPassCallbackError{std::move(image).error()}
                        };

                    const std::vector<glm::uvec2> extent{
                        {
                            (*image)->format.width,
                            (*image)->format.height
                        }
                    };

                    auto bind = context.driver.commandBindGraphicsPipeline(
                        context.commandBuffer,
                        pipeline
                    );
                    if (!bind)
                        return std::unexpected{
                            RenderPass::RenderPassCallbackError{std::move(bind).error()}
                        };

                    if (auto result = context.driver.commandSetViewport(
                            context.commandBuffer,
                            extent
                        ); !result)
                        return std::unexpected{
                            RenderPass::RenderPassCallbackError{std::move(result).error()}
                        };

                    if (auto result = context.driver.commandSetScissor(
                            context.commandBuffer,
                            extent
                        ); !result)
                        return std::unexpected{
                            RenderPass::RenderPassCallbackError{std::move(result).error()}
                        };

                    if (auto result = context.driver.commandDraw(
                            context.commandBuffer,
                            3,
                            1,
                            0,
                            0
                        ); !result)
                        return std::unexpected{
                            RenderPass::RenderPassCallbackError{std::move(result).error()}
                        };

                    return {};
                }
            );

            auto graph = require(std::move(builder).build(device));
            if (auto result = device.executeFrameGraph(graph);
                !result) {
                spdlog::error("{}", result.error().message);

                if (result.error().cause)
                    std::rethrow_exception(result.error().cause);

                throw std::runtime_error(result.error().message);
            }
        };

        while (!display.shouldClose(window))
            display.update(window, draw);
    } catch (const std::exception& e) {
        spdlog::error("{}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
