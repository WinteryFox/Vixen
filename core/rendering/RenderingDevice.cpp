#include "RenderingDevice.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <format>
#include <new>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <spdlog/spdlog.h>

#include "RenderingContextDriver.h"
#include "RenderingDeviceDriver.h"
#include "core/error/CantCreateError.h"
#include "core/error/Macros.h"
#include "core/error/SwapchainError.h"
#include "pipeline/GraphicsPipelineDescription.h"
#include "pipeline/PipelineLayout.h"
#include "shader/Shader.h"

namespace Vixen {
    namespace {
        struct DescriptorCounts {
            uint64_t samplers = 0;
            uint64_t uniformBuffers = 0;
            uint64_t storageBuffers = 0;
            uint64_t sampledImages = 0;
            uint64_t storageImages = 0;
            uint64_t inputAttachments = 0;
            uint64_t resources = 0;
        };

        constexpr std::array shaderStages{
            ShaderStageBits::Vertex,
            ShaderStageBits::Fragment,
            ShaderStageBits::TesselationControl,
            ShaderStageBits::TesselationEvaluation,
            ShaderStageBits::Compute,
            ShaderStageBits::Geometry
        };

        [[nodiscard]] constexpr std::string_view shaderStageName(const ShaderStageBits stage) noexcept {
            switch (stage) {
                case ShaderStageBits::Vertex:
                    return "vertex";
                case ShaderStageBits::Fragment:
                    return "fragment";
                case ShaderStageBits::TesselationControl:
                    return "tessellation-control";
                case ShaderStageBits::TesselationEvaluation:
                    return "tessellation-evaluation";
                case ShaderStageBits::Compute:
                    return "compute";
                case ShaderStageBits::Geometry:
                    return "geometry";
            }

            return "unrecognized";
        }

        template <typename Enum>
        [[nodiscard]] constexpr bool isRecognizedEnumValue(
            const Enum value,
            const Enum last
        ) noexcept {
            const auto rawValue = static_cast<int64_t>(value);
            return rawValue >= 0 && rawValue <= static_cast<int64_t>(last);
        }

        [[nodiscard]] auto validateShaderLayoutCompatibility(
            const Shader& shader,
            const PipelineLayout& layout
        ) -> std::expected<void, ResourceCreationError> {
            const auto& layoutDescription = layout.getDescription();

            for (const auto& uniform : shader.uniformSets) {
                const auto descriptorSet = std::ranges::find_if(
                    layoutDescription.descriptorSets,
                    [&](const DescriptorSetLayoutDescription& candidate) {
                        return candidate.set == uniform.set;
                    }
                );
                if (descriptorSet == layoutDescription.descriptorSets.end())
                    return std::unexpected{
                        ResourceCreationError{
                            .code = ResourceCreationErrorCode::CompatibilityError,
                            .message = "Pipeline layout is missing a descriptor set required by the shader",
                            .details = {std::format("Descriptor set: {}", uniform.set)}
                        }
                    };

                const auto binding = std::ranges::find_if(
                    descriptorSet->bindings,
                    [&](const DescriptorBindingLayout& candidate) {
                        return candidate.binding == uniform.binding;
                    }
                );
                const auto context = std::format(
                    "Descriptor set {}, binding {}",
                    uniform.set,
                    uniform.binding
                );
                if (binding == descriptorSet->bindings.end())
                    return std::unexpected{
                        ResourceCreationError{
                            .code = ResourceCreationErrorCode::CompatibilityError,
                            .message = "Pipeline layout is missing a descriptor binding required by the shader",
                            .details = {context}
                        }
                    };

                if (binding->type != uniform.type)
                    return std::unexpected{
                        ResourceCreationError{
                            .code = ResourceCreationErrorCode::CompatibilityError,
                            .message = "Pipeline layout descriptor type does not match the shader",
                            .details = {
                                context,
                                std::format(
                                    "Layout type: {}; shader type: {}",
                                    static_cast<uint32_t>(binding->type),
                                    static_cast<uint32_t>(uniform.type)
                                )
                            }
                        }
                    };

                if (binding->count < uniform.count)
                    return std::unexpected{
                        ResourceCreationError{
                            .code = ResourceCreationErrorCode::CompatibilityError,
                            .message = "Pipeline layout descriptor count is smaller than the shader requires",
                            .details = {
                                context,
                                std::format(
                                    "Layout count: {}; shader-required count: {}",
                                    binding->count,
                                    uniform.count
                                )
                            }
                        }
                    };

                const uint32_t missingStages = uniform.stages.value() & ~binding->stages.value();
                if (missingStages != 0)
                    return std::unexpected{
                        ResourceCreationError{
                            .code = ResourceCreationErrorCode::CompatibilityError,
                            .message = "Pipeline layout descriptor is not visible to every shader stage that uses it",
                            .details = {
                                context,
                                std::format("Missing shader-stage mask: 0x{:X}", missingStages)
                            }
                        }
                    };
            }

            if ((shader.pushConstantSize == 0) != shader.pushConstantStages.empty())
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::InvalidDescription,
                        .message = "Shader contains inconsistent push-constant reflection metadata"
                    }
                };

            if (shader.pushConstantSize == 0)
                return {};

            uint32_t recognizedPushConstantStages = 0;
            for (const ShaderStageBits stage : shaderStages) {
                if (!shader.pushConstantStages.contains(stage))
                    continue;

                recognizedPushConstantStages |= static_cast<uint32_t>(stage);
                const auto range = std::ranges::find_if(
                    layoutDescription.pushConstantRanges,
                    [stage](const PushConstantRange& candidate) {
                        return candidate.stages.contains(stage);
                    }
                );
                if (range == layoutDescription.pushConstantRanges.end())
                    return std::unexpected{
                        ResourceCreationError{
                            .code = ResourceCreationErrorCode::CompatibilityError,
                            .message = "Pipeline layout has no push-constant range for a shader stage",
                            .details = {std::format("Shader stage: {}", shaderStageName(stage))}
                        }
                    };

                const uint64_t rangeEnd = static_cast<uint64_t>(range->offset) + range->size;
                if (range->offset != 0 || rangeEnd < shader.pushConstantSize)
                    return std::unexpected{
                        ResourceCreationError{
                            .code = ResourceCreationErrorCode::CompatibilityError,
                            .message = "Pipeline layout push-constant range does not cover the shader block",
                            .details = {
                                std::format("Shader stage: {}", shaderStageName(stage)),
                                std::format("Shader byte range: [0, {})", shader.pushConstantSize),
                                std::format("Layout byte range: [{}, {})", range->offset, rangeEnd)
                            }
                        }
                    };
            }

            const uint32_t unknownStages = shader.pushConstantStages.value() & ~recognizedPushConstantStages;
            if (unknownStages != 0)
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::InvalidDescription,
                        .message = "Shader push-constant metadata contains unrecognized shader-stage bits",
                        .details = {std::format("Unrecognized stage mask: 0x{:X}", unknownStages)}
                    }
                };

            return {};
        }

        [[nodiscard]] auto validateGraphicsPipelineState(
            const GraphicsPipelineDescription& description
        ) -> std::expected<void, ResourceCreationError> {
            const auto validateEnum = []<typename Enum>(
                const Enum value,
                const Enum last,
                const std::string_view field
            ) -> std::expected<void, ResourceCreationError> {
                if (isRecognizedEnumValue(value, last))
                    return {};

                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::InvalidDescription,
                        .message = std::format("Graphics pipeline contains an unrecognized {} value", field),
                        .details = {std::format("Numeric value: {}", static_cast<int64_t>(value))}
                    }
                };
            };

            if (auto result = validateEnum(
                description.topology,
                PrimitiveTopology::TriangleFan,
                "primitive topology"
            ); !result)
                return result;

            if (description.isPrimitiveRestartEnabled &&
                description.topology != PrimitiveTopology::LineStrip &&
                description.topology != PrimitiveTopology::TriangleStrip &&
                description.topology != PrimitiveTopology::TriangleFan)
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::UnsupportedUsage,
                        .message = "Primitive restart is only supported for strip and fan topologies"
                    }
                };

            constexpr uint32_t supportedDynamicStateMask =
                static_cast<uint32_t>(DynamicStateBits::Viewport) |
                static_cast<uint32_t>(DynamicStateBits::Scissor) |
                static_cast<uint32_t>(DynamicStateBits::BlendConstants);
            const uint32_t unsupportedDynamicStates = description.dynamicStates.value() & ~supportedDynamicStateMask;
            if (unsupportedDynamicStates != 0)
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::InvalidDescription,
                        .message = "Graphics pipeline contains unsupported dynamic-state bits",
                        .details = {std::format("Unsupported dynamic-state mask: 0x{:X}", unsupportedDynamicStates)}
                    }
                };

            if (!description.dynamicStates.contains(DynamicStateBits::Viewport) ||
                !description.dynamicStates.contains(DynamicStateBits::Scissor))
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::InvalidDescription,
                        .message = "Graphics pipelines must use dynamic viewport and scissor state",
                        .details = {"Static viewport and scissor descriptions are not currently representable"}
                    }
                };

            for (size_t bindingIndex = 0; bindingIndex < description.vertexBindings.size(); ++bindingIndex)
                if (auto result = validateEnum(
                    description.vertexBindings[bindingIndex].rate,
                    InputRate::Instance,
                    "vertex-input rate"
                ); !result) {
                    auto error = std::move(result).error();
                    error.details.push_back(std::format("Vertex binding: {}", bindingIndex));
                    return std::unexpected{std::move(error)};
                }

            if (auto result = validateEnum(
                description.rasterization.polygonMode,
                PolygonMode::Point,
                "polygon mode"
            ); !result)
                return result;
            if (auto result = validateEnum(
                description.rasterization.cullMode,
                CullMode::FrontAndBack,
                "cull mode"
            ); !result)
                return result;
            if (auto result = validateEnum(
                description.rasterization.frontFace,
                FrontFace::Clockwise,
                "front-face winding"
            ); !result)
                return result;
            if (auto result = validateEnum(
                description.multisampling.samples,
                ImageSamples::SixtyFour,
                "sample count"
            ); !result)
                return result;

            if (!std::isfinite(description.rasterization.depthBiasConstantFactor) ||
                !std::isfinite(description.rasterization.depthBiasClamp) ||
                !std::isfinite(description.rasterization.depthBiasSlopeFactor))
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::InvalidDescription,
                        .message = "Graphics pipeline depth-bias values must be finite"
                    }
                };

            if (!std::isfinite(description.rasterization.lineWidth) ||
                description.rasterization.lineWidth <= 0.0f)
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::InvalidDescription,
                        .message = "Graphics pipeline line width must be finite and greater than zero",
                        .details = {std::format("Line width: {}", description.rasterization.lineWidth)}
                    }
                };

            if (!std::isfinite(description.multisampling.minSampleShading) ||
                description.multisampling.minSampleShading < 0.0f ||
                description.multisampling.minSampleShading > 1.0f)
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::InvalidDescription,
                        .message = "Graphics pipeline minimum sample shading must be between zero and one",
                        .details = {
                            std::format("Minimum sample shading: {}", description.multisampling.minSampleShading)
                        }
                    }
                };

            if (description.rasterization.isDepthClampEnabled)
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::UnsupportedUsage,
                        .message = "Depth clamping is not enabled by the rendering backend"
                    }
                };
            if (description.rasterization.polygonMode != PolygonMode::Fill)
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::UnsupportedUsage,
                        .message = "Non-solid polygon modes are not enabled by the rendering backend"
                    }
                };
            if (description.rasterization.depthBiasClamp != 0.0f)
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::UnsupportedUsage,
                        .message = "Depth-bias clamping is not enabled by the rendering backend"
                    }
                };
            if (description.rasterization.lineWidth != 1.0f)
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::UnsupportedUsage,
                        .message = "Wide lines are not enabled by the rendering backend",
                        .details = {std::format("Requested line width: {}", description.rasterization.lineWidth)}
                    }
                };
            if (description.multisampling.isSampleShadingEnabled)
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::UnsupportedUsage,
                        .message = "Sample-rate shading is not enabled by the rendering backend"
                    }
                };
            if (description.multisampling.isAlphaToOneEnabled)
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::UnsupportedUsage,
                        .message = "Alpha-to-one multisampling is not enabled by the rendering backend"
                    }
                };
            if (description.depthStencil.isDepthBoundsTestEnabled)
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::UnsupportedUsage,
                        .message = "Depth-bounds testing is not enabled by the rendering backend"
                    }
                };

            if (!std::isfinite(description.depthStencil.minDepthBounds) ||
                !std::isfinite(description.depthStencil.maxDepthBounds) ||
                description.depthStencil.minDepthBounds < 0.0f ||
                description.depthStencil.maxDepthBounds > 1.0f ||
                description.depthStencil.minDepthBounds > description.depthStencil.maxDepthBounds)
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::InvalidDescription,
                        .message = "Graphics pipeline depth bounds must be ordered values between zero and one",
                        .details = {
                            std::format(
                                "Depth bounds: [{}, {}]",
                                description.depthStencil.minDepthBounds,
                                description.depthStencil.maxDepthBounds
                            )
                        }
                    }
                };

            if (auto result = validateEnum(
                description.depthStencil.compareOperator,
                CompareOperator::Always,
                "depth compare operator"
            ); !result)
                return result;

            const auto validateStencilState = [&](
                const StencilOperatorState& state,
                const std::string_view face
            ) -> std::expected<void, ResourceCreationError> {
                const std::array results{
                    validateEnum(state.failOperator, StencilOperator::DecrementAndWrap, "stencil fail operator"),
                    validateEnum(state.passOperator, StencilOperator::DecrementAndWrap, "stencil pass operator"),
                    validateEnum(
                        state.depthFailOperator,
                        StencilOperator::DecrementAndWrap,
                        "stencil depth-fail operator"
                    ),
                    validateEnum(state.compareOperator, CompareOperator::Always, "stencil compare operator")
                };
                for (const auto& result : results)
                    if (!result) {
                        auto error = result.error();
                        error.details.push_back(std::format("Stencil face: {}", face));
                        return std::unexpected{std::move(error)};
                    }

                return {};
            };

            if (auto result = validateStencilState(description.depthStencil.front, "front"); !result)
                return result;
            if (auto result = validateStencilState(description.depthStencil.back, "back"); !result)
                return result;

            const bool hasDepth = description.depthStencilFormat && hasDepthAspect(*description.depthStencilFormat);
            const bool hasStencil = description.depthStencilFormat && hasStencilAspect(*description.depthStencilFormat);
            if ((description.depthStencil.isDepthTestEnabled || description.depthStencil.isDepthWriteEnabled ||
                 description.depthStencil.isDepthBoundsTestEnabled) && !hasDepth)
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::CompatibilityError,
                        .message = "Graphics pipeline enables depth operations without a depth attachment format"
                    }
                };
            if (description.depthStencil.isStencilTestEnabled && !hasStencil)
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::CompatibilityError,
                        .message = "Graphics pipeline enables stencil testing without a stencil attachment format"
                    }
                };

            constexpr uint32_t supportedColorWriteMask =
                static_cast<uint32_t>(ColorComponentBits::Red) |
                static_cast<uint32_t>(ColorComponentBits::Green) |
                static_cast<uint32_t>(ColorComponentBits::Blue) |
                static_cast<uint32_t>(ColorComponentBits::Alpha);
            for (size_t attachmentIndex = 0; attachmentIndex < description.colorBlending.size(); ++attachmentIndex) {
                const auto& attachment = description.colorBlending[attachmentIndex];
                const uint32_t unsupportedColorComponents = attachment.colorWriteMask.value() & ~supportedColorWriteMask;
                if (unsupportedColorComponents != 0)
                    return std::unexpected{
                        ResourceCreationError{
                            .code = ResourceCreationErrorCode::InvalidDescription,
                            .message = "Graphics pipeline color-write mask contains unsupported bits",
                            .details = {
                                std::format("Color attachment: {}", attachmentIndex),
                                std::format("Unsupported color-component mask: 0x{:X}", unsupportedColorComponents)
                            }
                        }
                    };

                const std::array results{
                    validateEnum(attachment.sourceColorBlendFactor, BlendFactor::SrcAlphaSaturate, "source color blend factor"),
                    validateEnum(attachment.destinationColorBlendFactor, BlendFactor::SrcAlphaSaturate, "destination color blend factor"),
                    validateEnum(attachment.colorBlendOperation, BlendOperation::Max, "color blend operation"),
                    validateEnum(attachment.sourceAlphaBlendFactor, BlendFactor::SrcAlphaSaturate, "source alpha blend factor"),
                    validateEnum(attachment.destinationAlphaBlendFactor, BlendFactor::SrcAlphaSaturate, "destination alpha blend factor"),
                    validateEnum(attachment.alphaBlendOperation, BlendOperation::Max, "alpha blend operation")
                };
                for (const auto& result : results)
                    if (!result) {
                        auto error = result.error();
                        error.details.push_back(std::format("Color attachment: {}", attachmentIndex));
                        return std::unexpected{std::move(error)};
                    }
            }

            return {};
        }

        void addDescriptorCount(
            DescriptorCounts& counts,
            const ShaderUniformType type,
            const uint32_t count
        ) noexcept {
            switch (type) {
                case ShaderUniformType::Sampler:
                    counts.samplers += count;
                    return;

                case ShaderUniformType::SampledImage:
                case ShaderUniformType::UniformTexelBuffer:
                    counts.sampledImages += count;
                    break;

                case ShaderUniformType::CombinedImageSampler:
                    counts.samplers += count;
                    counts.sampledImages += count;
                    break;

                case ShaderUniformType::StorageImage:
                case ShaderUniformType::StorageTexelBuffer:
                    counts.storageImages += count;
                    break;

                case ShaderUniformType::UniformBuffer:
                    counts.uniformBuffers += count;
                    break;

                case ShaderUniformType::StorageBuffer:
                    counts.storageBuffers += count;
                    break;

                case ShaderUniformType::InputAttachment:
                    counts.inputAttachments += count;
                    break;
            }

            // Vulkan excludes standalone sampler descriptors from maxPerStageResources.
            counts.resources += count;
        }

        [[nodiscard]] auto validateDescriptorCounts(
            const DescriptorCounts& counts,
            const DescriptorCountLimits& limits,
            const bool perStage,
            const std::string_view stageName = {}
        ) -> std::expected<void, ResourceCreationError> {
            const auto validate = [perStage, stageName](
                const uint64_t requested,
                const uint64_t supported,
                const std::string_view descriptorType,
                const std::string_view totalLimit,
                const std::string_view perStageLimit
            ) -> std::expected<void, ResourceCreationError> {
                if (requested <= supported)
                    return {};

                ResourceCreationError error{
                    .code = ResourceCreationErrorCode::ExceedsDeviceLimits,
                    .message = std::format(
                        "Pipeline layout exceeds the {}{} descriptor limit",
                        perStage ? "per-stage " : "",
                        descriptorType
                    ),
                    .limitViolation = ResourceCreationLimitViolation{
                        .limit = std::string{perStage ? perStageLimit : totalLimit},
                        .requested = requested,
                        .supported = supported
                    }
                };
                if (perStage)
                    error.details.push_back(std::format("Shader stage: {}", stageName));

                return std::unexpected{std::move(error)};
            };

            if (auto result = validate(
                counts.samplers,
                limits.samplers,
                "sampler",
                "maxDescriptorSetSamplers",
                "maxPerStageDescriptorSamplers"
            ); !result)
                return result;

            if (auto result = validate(
                counts.uniformBuffers,
                limits.uniformBuffers,
                "uniform-buffer",
                "maxDescriptorSetUniformBuffers",
                "maxPerStageDescriptorUniformBuffers"
            ); !result)
                return result;

            if (auto result = validate(
                counts.storageBuffers,
                limits.storageBuffers,
                "storage-buffer",
                "maxDescriptorSetStorageBuffers",
                "maxPerStageDescriptorStorageBuffers"
            ); !result)
                return result;

            if (auto result = validate(
                counts.sampledImages,
                limits.sampledImages,
                "sampled-image",
                "maxDescriptorSetSampledImages",
                "maxPerStageDescriptorSampledImages"
            ); !result)
                return result;

            if (auto result = validate(
                counts.storageImages,
                limits.storageImages,
                "storage-image",
                "maxDescriptorSetStorageImages",
                "maxPerStageDescriptorStorageImages"
            ); !result)
                return result;

            return validate(
                counts.inputAttachments,
                limits.inputAttachments,
                "input-attachment",
                "maxDescriptorSetInputAttachments",
                "maxPerStageDescriptorInputAttachments"
            );
        }

        [[nodiscard]] auto validatePipelineLayoutDescription(
            const PipelineLayoutDescription& description,
            const PipelineLayoutLimits& limits
        ) -> std::expected<void, ResourceCreationError> {
            constexpr uint32_t supportedShaderStageMask =
                static_cast<uint32_t>(ShaderStageBits::Vertex) |
                static_cast<uint32_t>(ShaderStageBits::Fragment) |
                static_cast<uint32_t>(ShaderStageBits::TesselationControl) |
                static_cast<uint32_t>(ShaderStageBits::TesselationEvaluation) |
                static_cast<uint32_t>(ShaderStageBits::Compute) |
                static_cast<uint32_t>(ShaderStageBits::Geometry);

            const auto validateStageMask = [](
                const ShaderStageFlags stages,
                std::string context
            ) -> std::expected<void, ResourceCreationError> {
                if (stages.empty())
                    return std::unexpected{
                        ResourceCreationError{
                            .code = ResourceCreationErrorCode::InvalidDescription,
                            .message = "Pipeline layout contains an empty shader-stage mask",
                            .details = {std::move(context)}
                        }
                    };

                const uint32_t unsupportedStages = stages.value() & ~supportedShaderStageMask;
                if (unsupportedStages != 0)
                    return std::unexpected{
                        ResourceCreationError{
                            .code = ResourceCreationErrorCode::InvalidDescription,
                            .message = "Pipeline layout contains unsupported shader-stage bits",
                            .details = {
                                std::move(context),
                                std::format("Unsupported stage mask: 0x{:X}", unsupportedStages)
                            }
                        }
                    };

                return {};
            };

            std::unordered_set<uint32_t> descriptorSetNumbers{};
            descriptorSetNumbers.reserve(description.descriptorSets.size());

            if (description.descriptorSets.size() > limits.maxBoundDescriptorSets)
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::ExceedsDeviceLimits,
                        .message = "Pipeline layout contains too many descriptor sets",
                        .limitViolation = ResourceCreationLimitViolation{
                            .limit = "maxBoundDescriptorSets",
                            .requested = description.descriptorSets.size(),
                            .supported = limits.maxBoundDescriptorSets
                        }
                    }
                };

            DescriptorCounts totalDescriptorCounts{};
            std::array<DescriptorCounts, shaderStages.size()> perStageDescriptorCounts{};

            for (const auto& descriptorSet : description.descriptorSets) {
                if (descriptorSet.set >= limits.maxBoundDescriptorSets)
                    return std::unexpected{
                        ResourceCreationError{
                            .code = ResourceCreationErrorCode::ExceedsDeviceLimits,
                            .message = "Pipeline layout descriptor-set index exceeds the device limit",
                            .limitViolation = ResourceCreationLimitViolation{
                                .limit = "maxBoundDescriptorSets",
                                .requested = static_cast<uint64_t>(descriptorSet.set) + 1,
                                .supported = limits.maxBoundDescriptorSets
                            },
                            .details = {std::format("Descriptor set {}", descriptorSet.set)}
                        }
                    };

                if (!descriptorSetNumbers.insert(descriptorSet.set).second)
                    return std::unexpected{
                        ResourceCreationError{
                            .code = ResourceCreationErrorCode::InvalidDescription,
                            .message = "Pipeline layout contains a duplicate descriptor-set number",
                            .details = {std::format("Descriptor set {}", descriptorSet.set)}
                        }
                    };

                std::unordered_set<uint32_t> bindingNumbers{};
                bindingNumbers.reserve(descriptorSet.bindings.size());

                for (const auto& binding : descriptorSet.bindings) {
                    const auto context = std::format(
                        "Descriptor set {}, binding {}",
                        descriptorSet.set,
                        binding.binding
                    );

                    if (!bindingNumbers.insert(binding.binding).second)
                        return std::unexpected{
                            ResourceCreationError{
                                .code = ResourceCreationErrorCode::InvalidDescription,
                                .message = "Pipeline layout descriptor set contains a duplicate binding number",
                                .details = {context}
                            }
                        };

                    if (binding.count == 0)
                        return std::unexpected{
                            ResourceCreationError{
                                .code = ResourceCreationErrorCode::InvalidDescription,
                                .message = "Pipeline layout descriptor binding has a descriptor count of zero",
                                .details = {context}
                            }
                        };

                    if (auto stages = validateStageMask(binding.stages, context); !stages)
                        return std::unexpected{std::move(stages).error()};

                    if (!binding.immutableSamplers.empty()) {
                        if (binding.type != ShaderUniformType::Sampler &&
                            binding.type != ShaderUniformType::CombinedImageSampler)
                            return std::unexpected{
                                ResourceCreationError{
                                    .code = ResourceCreationErrorCode::InvalidDescription,
                                    .message =
                                    "Pipeline layout immutable samplers require a sampler or combined-image-sampler descriptor",
                                    .details = {context}
                                }
                            };

                        if (binding.immutableSamplers.size() != binding.count)
                            return std::unexpected{
                                ResourceCreationError{
                                    .code = ResourceCreationErrorCode::InvalidDescription,
                                    .message =
                                    "Pipeline layout immutable-sampler count does not match the descriptor count",
                                    .details = {
                                        context,
                                        std::format(
                                            "Descriptor count: {}; immutable-sampler count: {}",
                                            binding.count,
                                            binding.immutableSamplers.size()
                                        )
                                    }
                                }
                            };
                    }

                    addDescriptorCount(totalDescriptorCounts, binding.type, binding.count);
                    for (size_t stageIndex = 0; stageIndex < shaderStages.size(); ++stageIndex)
                        if (binding.stages.contains(shaderStages[stageIndex]))
                            addDescriptorCount(
                                perStageDescriptorCounts[stageIndex],
                                binding.type,
                                binding.count
                            );
                }
            }

            if (auto validation = validateDescriptorCounts(
                totalDescriptorCounts,
                limits.maxDescriptors,
                false
            ); !validation)
                return validation;

            for (size_t stageIndex = 0; stageIndex < shaderStages.size(); ++stageIndex) {
                const auto& counts = perStageDescriptorCounts[stageIndex];
                if (auto validation = validateDescriptorCounts(
                    counts,
                    limits.maxPerStageDescriptors,
                    true,
                    shaderStageName(shaderStages[stageIndex])
                ); !validation)
                    return validation;

                if (counts.resources > limits.maxPerStageResources)
                    return std::unexpected{
                        ResourceCreationError{
                            .code = ResourceCreationErrorCode::ExceedsDeviceLimits,
                            .message = "Pipeline layout exceeds the per-stage resource limit",
                            .limitViolation = ResourceCreationLimitViolation{
                                .limit = "maxPerStageResources",
                                .requested = counts.resources,
                                .supported = limits.maxPerStageResources
                            },
                            .details = {
                                std::format("Shader stage: {}", shaderStageName(shaderStages[stageIndex]))
                            }
                        }
                    };
            }

            for (size_t rangeIndex = 0; rangeIndex < description.pushConstantRanges.size(); ++rangeIndex) {
                const auto& range = description.pushConstantRanges[rangeIndex];
                const auto context = std::format("Push-constant range {}", rangeIndex);

                if (range.offset % 4 != 0)
                    return std::unexpected{
                        ResourceCreationError{
                            .code = ResourceCreationErrorCode::InvalidDescription,
                            .message = "Pipeline layout push-constant range offset is not four-byte aligned",
                            .details = {
                                context,
                                std::format("Offset: {}", range.offset)
                            }
                        }
                    };

                if (range.size == 0 || range.size % 4 != 0)
                    return std::unexpected{
                        ResourceCreationError{
                            .code = ResourceCreationErrorCode::InvalidDescription,
                            .message = range.size == 0
                                           ? "Pipeline layout push-constant range has a size of zero"
                                           : "Pipeline layout push-constant range size is not a multiple of four",
                            .details = {
                                context,
                                std::format("Size: {}", range.size)
                            }
                        }
                    };

                if (auto stages = validateStageMask(range.stages, context); !stages)
                    return std::unexpected{std::move(stages).error()};

                for (size_t otherIndex = 0; otherIndex < rangeIndex; ++otherIndex) {
                    const auto& other = description.pushConstantRanges[otherIndex];
                    if (!(range.stages & other.stages).empty())
                        return std::unexpected{
                            ResourceCreationError{
                                .code = ResourceCreationErrorCode::InvalidDescription,
                                .message =
                                "Pipeline layout assigns a shader stage to more than one push-constant range",
                                .details = {
                                    std::format("Push-constant ranges {} and {}", otherIndex, rangeIndex),
                                    std::format(
                                        "Byte ranges: [{}, {}) and [{}, {})",
                                        other.offset,
                                        static_cast<uint64_t>(other.offset) + other.size,
                                        range.offset,
                                        static_cast<uint64_t>(range.offset) + range.size
                                    )
                                }
                            }
                        };
                }
            }

            return {};
        }
    }

    void RenderingDevice::waitForFrame(
        const uint32_t frameIndex
    ) {
        if (!frames[frameIndex].fenceSignaled)
            return;

        renderingDeviceDriver->waitOnFence(frames[frameIndex].fence).value();
        frames[frameIndex].fenceSignaled = false;
    }

    void RenderingDevice::waitForFrames() {
        for (uint32_t i = 0; i < frames.size(); i++)
            waitForFrame(i);
    }

    void RenderingDevice::drainDeferredReleases(Frame& frame) {
        auto releases = std::move(frame.deferredReleases);

        frame.deferredReleases.clear();

        for (auto& release : releases)
            release(*renderingDeviceDriver);
    }

    void RenderingDevice::flushAndWaitForFrames() {
        waitForFrames();
        endFrame();
        executeFrame(false);
        beginFrame(false);
    }

    void RenderingDevice::beginFrame(
        const bool presented
    ) {
        waitForFrame(frameIndex);
        drainDeferredReleases(frames[frameIndex]);

        if (!renderingDeviceDriver->resetCommandPool(frames[frameIndex].commandPool))
            throw std::runtime_error("Failed to reset command pool");
        if (!renderingDeviceDriver->beginCommandBuffer(frames[frameIndex].commandBuffer))
            throw std::runtime_error("Failed to begin command buffer");
    }

    void RenderingDevice::endFrame() {
        renderingDeviceDriver->endCommandBuffer(frames[frameIndex].commandBuffer);
    }

    void RenderingDevice::executeChainedCommands(
        const bool present,
        Fence* drawFence,
        Semaphore* drawSemaphoreToSignal
    ) {
        if (!renderingDeviceDriver->executeCommandQueueAndPresent(
            graphicsQueue,
            frames[frameIndex].waitSemaphores,
            frames[frameIndex].commandBuffer
                ? std::vector{frames[frameIndex].commandBuffer}
                : std::vector<CommandBuffer*>{},
            drawSemaphoreToSignal
                ? std::vector{drawSemaphoreToSignal}
                : std::vector<Semaphore*>{},
            drawFence,
            present
                ? frames[frameIndex].swapchainsToPresent
                : std::vector<Swapchain*>{}
        ))
            throw std::runtime_error("Failed to execute chained commands");

        frames[frameIndex].waitSemaphores.clear();
    }

    void RenderingDevice::executeFrame(
        const bool present
    ) {
        const bool canPresent = present && !frames[frameIndex].swapchainsToPresent.empty();

        executeChainedCommands(canPresent, frames[frameIndex].fence, nullptr);
        frames[frameIndex].fenceSignaled = true;

        if (canPresent)
            frames[frameIndex].swapchainsToPresent.clear();
    }

    RenderingDevice::RenderingDevice(
        RenderingContextDriver* renderingContext,
        Window* mainWindow
    ) : renderingContextDriver(renderingContext),
        frameIndex(0) {
        Surface* mainSurface = renderingContextDriver->getSurfaceFromWindow(mainWindow);

        const auto devices = renderingContextDriver->getDevices();

        std::string deviceList;
        for (uint32_t i = 0; i < devices.size(); ++i) {
            if (!deviceList.empty())
                deviceList += '\n';

            deviceList += std::format(
                "    [{}] - {}\n"
                "            * Supports presentation? {}",
                i,
                devices[i].name,
                renderingContext->deviceSupportsPresent(i, mainSurface) ? "Yes" : "No"
            );
        }
        spdlog::trace("Found the following devices.\n{}", deviceList);

        uint32_t deviceIndex = std::numeric_limits<uint32_t>::max();
        uint64_t bestDeviceScore = 0;
        for (uint32_t i = 0; i < devices.size(); i++) {
            const auto& deviceOption = devices[i];
            const bool supportsPresent = mainSurface != nullptr
                                             ? renderingContext->deviceSupportsPresent(i, mainSurface)
                                             : false;

            if (!supportsPresent)
                continue;

            uint64_t score = 1;
            switch (deviceOption.type) {
                case DriverDeviceType::Discrete:
                    score += 1'000'000;
                    break;
                case DriverDeviceType::Integrated:
                    score += 500'000;
                    break;
                case DriverDeviceType::Virtual:
                    score += 250'000;
                    break;
                case DriverDeviceType::Cpu:
                    score += 100'000;
                    break;
                case DriverDeviceType::Other:
                    break;
            }

            constexpr uint64_t mebibyte = 1024 * 1024;
            score += std::min(deviceOption.deviceLocalMemory / mebibyte, 100'000ull);
            score += deviceOption.hasDedicatedComputeQueue ? 25'000 : 0;
            score += deviceOption.hasDedicatedTransferQueue ? 25'000 : 0;

            if (deviceIndex == std::numeric_limits<uint32_t>::max() || score > bestDeviceScore) {
                deviceIndex = i;
                bestDeviceScore = score;
            }
        }

        if (deviceIndex == std::numeric_limits<uint32_t>::max())
            error<CantCreateError>("No suitable device found.");

        uint32_t frameCount = 2;

        device = devices[deviceIndex];
        renderingDeviceDriver = renderingContext->createRenderingDeviceDriver(deviceIndex, frameCount);

        graphicsQueueFamily = renderingDeviceDriver->getQueueFamily(
            QueueFamilyBits::Graphics | QueueFamilyBits::Compute,
            nullptr
        ).value();
        graphicsQueue = renderingDeviceDriver->createCommandQueue(graphicsQueueFamily).value();

        transferQueueFamily = renderingDeviceDriver->getQueueFamily(QueueFamilyBits::Transfer, nullptr).value();
        transferQueue = renderingDeviceDriver->createCommandQueue(transferQueueFamily).value();

        frames.reserve(frameCount);
        for (uint32_t i = 0; i < frameCount; i++) {
            const auto commandPool = renderingDeviceDriver->createCommandPool(
                graphicsQueueFamily,
                CommandBufferType::Primary
            );
            if (!commandPool)
                throw CantCreateError("Failed to allocate command pool for frame");

            frames.push_back(
                {
                    .commandPool = commandPool.value(),
                    .commandBuffer = renderingDeviceDriver->createCommandBuffer(commandPool.value()).value(),
                    .fence = renderingDeviceDriver->createFence().value(),
                    .fenceSignaled = false,
                    .deferredReleases = {},
                    .waitSemaphores = {},
                    .swapchainsToPresent = {}
                }
            );
        }
        framesDrawn = frames.size();

        renderingDeviceDriver->beginCommandBuffer(frames[0].commandBuffer);
    }

    RenderingDevice::~RenderingDevice() {
        if (!frames.empty())
            flushAndWaitForFrames();

        for (auto& frame : frames)
            drainDeferredReleases(frame);

        for (const auto& frame : frames) {
            renderingDeviceDriver->destroyCommandPool(frame.commandPool);
            renderingDeviceDriver->destroyFence(frame.fence);
            delete frame.commandBuffer;
        }
        frames.clear();

        if (transferQueue)
            if (graphicsQueue != transferQueue)
                renderingDeviceDriver->destroyCommandQueue(transferQueue);

        if (graphicsQueue)
            renderingDeviceDriver->destroyCommandQueue(graphicsQueue);

        renderingContextDriver->destroyRenderingDeviceDriver(renderingDeviceDriver);
    }

    void RenderingDevice::swapBuffers(
        const bool present
    ) {
        endFrame();
        executeFrame(present);

        frameIndex = (frameIndex + 1) % frames.size();

        beginFrame(present);
    }

    void RenderingDevice::submit() {
        endFrame();
        executeFrame(false);
    }

    void RenderingDevice::sync() {
        beginFrame(true);
    }

    void RenderingDevice::deferRelease(DeferredRelease release) {
        if (release)
            frames[frameIndex].deferredReleases.push_back(std::move(release));
    }

    void RenderingDevice::deferDestroy(Image* image) {
        if (image == nullptr)
            return;

        deferRelease([image](RenderingDeviceDriver& driver) {
            driver.destroyImage(image);
        });
    }

    void RenderingDevice::deferDestroy(Buffer* buffer) {
        if (buffer == nullptr)
            return;

        deferRelease([buffer](RenderingDeviceDriver& driver) {
            driver.destroyBuffer(buffer);
        });
    }

    auto RenderingDevice::createScreen(
        Window* window
    ) -> std::expected<Swapchain*, Error> {
        const auto& surface = renderingContextDriver->getSurfaceFromWindow(window);
        if (surface == nullptr)
            return std::unexpected(Error::InitializationFailed);

        if (swapchains.contains(window))
            return std::unexpected(Error::InitializationFailed);

        const auto& swapchain = renderingDeviceDriver->createSwapchain(surface);
        if (!swapchain)
            return std::unexpected(Error::InitializationFailed);

        swapchains[window] = swapchain.value();

        return swapchain;
    }

    auto RenderingDevice::prepareScreenForDrawing(
        Window* window
    ) -> std::expected<Framebuffer*, Error> {
        const auto& pair = swapchains.find(window);
        DEBUG_ASSERT(pair != swapchains.end());
        const auto& swapchain = pair->second;

        uint32_t toPresentIndex = 0;
        while (toPresentIndex < frames[frameIndex].swapchainsToPresent.size()) {
            if (frames[frameIndex].swapchainsToPresent[toPresentIndex] == swapchain) {
                if (!renderingDeviceDriver->executeCommandQueueAndPresent(graphicsQueue, {}, {}, {}, {}, {swapchain}))
                    return std::unexpected(Error::InitializationFailed);

                frames[frameIndex].swapchainsToPresent.erase(
                    frames[frameIndex].swapchainsToPresent.begin() + toPresentIndex);
            } else {
                toPresentIndex++;
            }
        }

        auto framebuffer = renderingDeviceDriver->acquireSwapchainFramebuffer(graphicsQueue, swapchain);
        if (!framebuffer && framebuffer.error() == SwapchainError::ResizeRequired) {
            flushAndWaitForFrames();

            if (!renderingDeviceDriver->resizeSwapchain(graphicsQueue, swapchain, frames.size()))
                return std::unexpected(Error::InitializationFailed);

            framebuffer = renderingDeviceDriver->acquireSwapchainFramebuffer(graphicsQueue, swapchain);
        }

        if (!framebuffer)
            return std::unexpected(Error::InitializationFailed);

        frames[frameIndex].swapchainsToPresent.push_back(swapchain);

        return framebuffer.value();
    }

    void RenderingDevice::destroyScreen(
        Window* window
    ) {
        const auto& pair = swapchains.find(window);
        if (pair == swapchains.end())
            throw std::invalid_argument("Window does not have an associated swapchain");

        flushAndWaitForFrames();

        renderingDeviceDriver->destroySwapchain(pair->second);
        swapchains.erase(window);
    }

    auto RenderingDevice::createBuffer(
        const uint64_t size,
        const BufferUsageFlags usage,
        const MemoryAllocationType memoryType
    ) const -> std::expected<Buffer*, ResourceCreationError> {
        if (size == 0)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = "Cannot create a buffer with a size of 0"
                }
            };

        if (usage.empty())
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = "Cannot create a buffer with an empty usage mask"
                }
            };

        constexpr auto knownBufferUsages = BufferUsageBits::CopySource |
            BufferUsageBits::CopyDestination |
            BufferUsageBits::UniformTexel |
            BufferUsageBits::StorageTexel |
            BufferUsageBits::Uniform |
            BufferUsageBits::Storage |
            BufferUsageBits::Vertex |
            BufferUsageBits::Index |
            BufferUsageBits::Indirect;

        if (const auto unknownUsageBits = usage.value() & ~knownBufferUsages.value();
            unknownUsageBits != 0)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::UnsupportedUsage,
                    .message = std::format(
                        "Buffer usage contains unknown bits ({:#x})",
                        unknownUsageBits
                    )
                }
            };

        switch (memoryType) {
            case MemoryAllocationType::Cpu:
            case MemoryAllocationType::Gpu:
                break;

            default:
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::InvalidDescription,
                        .message = "Buffer memory allocation type is not recognized"
                    }
                };
        }

        const auto maxBufferSize = renderingDeviceDriver->getMaxBufferSize();
        if (size > maxBufferSize)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::ExceedsDeviceLimits,
                    .message = std::format(
                        "Buffer size of {} bytes exceeds the device limit of {} bytes",
                        size,
                        maxBufferSize
                    ),
                    .limitViolation = ResourceCreationLimitViolation{
                        .limit = "maxBufferSize",
                        .requested = size,
                        .supported = maxBufferSize
                    }
                }
            };

        const auto buffer = renderingDeviceDriver->createBuffer(
            size,
            usage,
            memoryType
        );
        if (!buffer)
            return std::unexpected{std::move(buffer).error()};

        if (*buffer == nullptr)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::NativeObjectCreationFailed,
                    .message = "The rendering backend reported successful buffer creation but returned a null buffer"
                }
            };

        return *buffer;
    }

    auto RenderingDevice::createVertexBuffer(
        const uint64_t size
    ) const -> std::expected<Buffer*, ResourceCreationError> {
        return createBuffer(
            size,
            BufferUsageBits::CopySource |
            BufferUsageBits::CopyDestination |
            BufferUsageBits::Vertex,
            MemoryAllocationType::Gpu
        );
    }

    auto RenderingDevice::createUniformBuffer(
        const uint64_t size
    ) const -> std::expected<Buffer*, ResourceCreationError> {
        return createBuffer(
            size,
            BufferUsageBits::CopyDestination |
            BufferUsageBits::Uniform,
            MemoryAllocationType::Gpu
        );
    }

    auto RenderingDevice::createStorageBuffer(
        const uint64_t size
    ) const -> std::expected<Buffer*, ResourceCreationError> {
        return createBuffer(
            size,
            BufferUsageBits::CopySource |
            BufferUsageBits::CopyDestination |
            BufferUsageBits::Storage,
            MemoryAllocationType::Gpu
        );
    }

    auto RenderingDevice::createTexelBuffer(
        const uint32_t elementCount,
        const ImageDataFormat format,
        const BufferUsageBits usage
    ) const -> std::expected<Buffer*, ResourceCreationError> {
        if (elementCount == 0)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = "A texel buffer must contain at least one texel"
                }
            };

        if (usage != BufferUsageBits::UniformTexel && usage != BufferUsageBits::StorageTexel)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = "A texel buffer must use either UniformTexel or StorageTexel usage"
                }
            };

        const auto texelSize = getTexelSize(format);
        if (texelSize == 0)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::UnsupportedFormat,
                    .message = "The requested format cannot be used as a texel-buffer format"
                }
            };

        const auto supportedUsages = renderingDeviceDriver->getTexelBufferUsageSupportedByFormat(format);
        if (!supportedUsages)
            return std::unexpected{std::move(supportedUsages).error()};

        if (!supportedUsages->contains(usage))
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::UnsupportedUsage,
                    .message = usage == BufferUsageBits::UniformTexel
                                   ? "Uniform texel-buffer access is not supported for the requested format"
                                   : "Storage texel-buffer access is not supported for the requested format"
                }
            };

        const auto maxElements = renderingDeviceDriver->getMaxTexelBufferElements();
        if (elementCount > maxElements)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::ExceedsDeviceLimits,
                    .message = std::format(
                        "Texel-buffer element count {} exceeds the device limit of {}",
                        elementCount,
                        maxElements
                    ),
                    .limitViolation = ResourceCreationLimitViolation{
                        .limit = "maxTexelBufferElements",
                        .requested = elementCount,
                        .supported = maxElements
                    }
                }
            };

        return createBuffer(
            static_cast<uint64_t>(elementCount) * texelSize,
            BufferUsageBits::CopySource |
            BufferUsageBits::CopyDestination |
            usage,
            MemoryAllocationType::Gpu
        );
    }

    auto RenderingDevice::createUniformTexelBuffer(
        const uint32_t elementCount,
        const ImageDataFormat format
    ) const -> std::expected<Buffer*, ResourceCreationError> {
        return createTexelBuffer(elementCount, format, BufferUsageBits::UniformTexel);
    }

    auto RenderingDevice::createStorageTexelBuffer(
        const uint32_t elementCount,
        const ImageDataFormat format
    ) const -> std::expected<Buffer*, ResourceCreationError> {
        return createTexelBuffer(elementCount, format, BufferUsageBits::StorageTexel);
    }

    auto RenderingDevice::createImage(
        const ImageFormat& format,
        const ImageView& view
    ) const -> std::expected<Image*, ResourceCreationError> {
        const auto invalidDescription = [](std::string message)
            -> std::expected<Image*, ResourceCreationError> {
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = std::move(message)
                }
            };
        };

        if (format.width == 0 ||
            format.height == 0 ||
            format.depth == 0 ||
            format.mipmapCount == 0 ||
            format.layerCount == 0)
            return invalidDescription(
                "Image width, height, depth, mipmap count, and layer count must all be greater than zero"
            );

        const auto maxMipmapCount = static_cast<uint32_t>(std::bit_width(
            std::max({format.width, format.height, format.depth})
        ));
        if (format.mipmapCount > maxMipmapCount)
            return invalidDescription(
                std::format(
                    "Image requests {} mip levels, but extent {}x{}x{} supports at most {}",
                    format.mipmapCount,
                    format.width,
                    format.height,
                    format.depth,
                    maxMipmapCount
                )
            );

        if (format.usage.empty())
            return invalidDescription("Image usage flags must not be empty");

        if (format.usage.value() == ImageUsageFlags{ImageUsageBits::CpuRead}.value())
            return invalidDescription(
                "CpuRead is a memory-placement capability and must be combined with a device image usage, "
                "such as CopyDestination for a readback image"
            );

        constexpr auto knownImageUsages = ImageUsageBits::Sampling |
            ImageUsageBits::ColorAttachment |
            ImageUsageBits::DepthStencilAttachment |
            ImageUsageBits::Storage |
            ImageUsageBits::AtomicStorage |
            ImageUsageBits::CpuRead |
            ImageUsageBits::Update |
            ImageUsageBits::CopySource |
            ImageUsageBits::CopyDestination |
            ImageUsageBits::InputAttachment |
            ImageUsageBits::TransientAttachment;

        if (const auto unknownUsageBits = format.usage.value() & ~knownImageUsages.value();
            unknownUsageBits != 0)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::UnsupportedUsage,
                    .message = std::format(
                        "Image usage contains unknown bits ({:#x})",
                        unknownUsageBits
                    )
                }
            };

        switch (format.type) {
            case ImageType::OneD:
                if (format.height != 1 || format.depth != 1)
                    return invalidDescription(
                        "One-dimensional images must have height 1 and depth 1"
                    );
                if (format.layerCount != 1)
                    return invalidDescription("Non-array one-dimensional images must have exactly one layer");
                break;

            case ImageType::OneDArray:
                if (format.height != 1 || format.depth != 1)
                    return invalidDescription(
                        "One-dimensional image arrays must have height 1 and depth 1"
                    );
                break;

            case ImageType::TwoD:
                if (format.depth != 1)
                    return invalidDescription("Two-dimensional images must have depth 1");
                if (format.layerCount != 1)
                    return invalidDescription("Non-array two-dimensional images must have exactly one layer");
                break;

            case ImageType::TwoDArray:
                if (format.depth != 1)
                    return invalidDescription("Two-dimensional image arrays must have depth 1");
                break;

            case ImageType::ThreeD:
                if (format.layerCount != 1)
                    return invalidDescription("Three-dimensional images must have exactly one array layer");
                break;

            case ImageType::Cube:
                if (format.depth != 1)
                    return invalidDescription("Cube images must have depth 1");
                if (format.width != format.height)
                    return invalidDescription("Cube images must have equal width and height");
                if (format.layerCount != 6)
                    return invalidDescription("Cube images must have exactly six array layers");
                break;

            case ImageType::CubeArray:
                if (format.depth != 1)
                    return invalidDescription("Cube-array images must have depth 1");
                if (format.width != format.height)
                    return invalidDescription("Cube-array images must have equal width and height");
                if (format.layerCount < 6 || format.layerCount % 6 != 0)
                    return invalidDescription(
                        "Cube-array images must have a positive multiple of six array layers"
                    );
                break;
        }

        if (format.samples != ImageSamples::One) {
            if (format.type != ImageType::TwoD && format.type != ImageType::TwoDArray)
                return invalidDescription(
                    "Multisampled images must be two-dimensional or two-dimensional arrays"
                );

            if (format.mipmapCount != 1)
                return invalidDescription("Multisampled images must have exactly one mip level");
        }

        const auto supportedUsages = renderingDeviceDriver->getImageUsageSupportedByFormat(
            format.format,
            format.usage.contains(ImageUsageBits::CpuRead)
        );
        if (!supportedUsages)
            return std::unexpected{std::move(supportedUsages).error()};

        if (format.usage.contains(ImageUsageBits::Sampling) &&
            !supportedUsages->contains(ImageUsageBits::Sampling))
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::UnsupportedUsage,
                    .message = "Sampling is not supported for this format"
                }
            };

        if (format.usage.contains(ImageUsageBits::ColorAttachment) &&
            !supportedUsages->contains(ImageUsageBits::ColorAttachment))
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::UnsupportedUsage,
                    .message = "Color attachment is not supported for this format"
                }
            };

        if (format.usage.contains(ImageUsageBits::DepthStencilAttachment) &&
            !supportedUsages->contains(ImageUsageBits::DepthStencilAttachment))
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::UnsupportedUsage,
                    .message = "Depth-stencil attachment is not supported for this format"
                }
            };

        if (format.usage.contains(ImageUsageBits::Storage) &&
            !supportedUsages->contains(ImageUsageBits::Storage))
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::UnsupportedUsage,
                    .message = "Storage is not supported for this format"
                }
            };

        if (format.usage.contains(ImageUsageBits::AtomicStorage) &&
            !supportedUsages->contains(ImageUsageBits::AtomicStorage))
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::UnsupportedUsage,
                    .message = "Atomic storage-image access is not supported for this format"
                }
            };

        const auto image = renderingDeviceDriver->createImage(format, view);
        if (!image)
            return std::unexpected{std::move(image).error()};

        if (*image == nullptr)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::NativeObjectCreationFailed,
                    .message = "The rendering backend reported successful image creation but returned a null pointer"
                }
            };

        return *image;
    }

    auto RenderingDevice::createPipelineLayout(
        const PipelineLayoutDescription& description
    ) const -> std::expected<PipelineLayout*, ResourceCreationError> try {
        if (auto validation = validatePipelineLayoutDescription(
            description,
            renderingDeviceDriver->getPipelineLayoutLimits()
        ); !validation)
            return std::unexpected{std::move(validation).error()};

        const auto layout = renderingDeviceDriver->createPipelineLayout(description);
        if (!layout)
            return std::unexpected{std::move(layout).error()};

        if (!*layout)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::NativeObjectCreationFailed,
                    .message =
                    "The rendering backend reported successful pipeline layout creation but returned a null pointer"
                }
            };

        return *layout;
    } catch (const std::bad_alloc&) {
        return std::unexpected{
            ResourceCreationError{
                .code = ResourceCreationErrorCode::OutOfHostMemory,
                .message = "Failed to allocate temporary storage while creating a pipeline layout"
            }
        };
    }

    auto RenderingDevice::createGraphicsPipeline(
        const GraphicsPipelineDescription& description
    ) const -> std::expected<GraphicsPipeline*, ResourceCreationError> try {
        if (description.shader == nullptr)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = "Graphics pipeline description does not specify a shader"
                }
            };

        if (description.layout == nullptr)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = "Graphics pipeline description does not specify a pipeline layout"
                }
            };

        if (description.shader->stages.contains(ShaderStageBits::Compute))
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = "Graphics pipeline shader contains a compute stage"
                }
            };

        if (!description.shader->stages.contains(ShaderStageBits::Vertex))
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = "Graphics pipeline shader does not contain a vertex stage"
                }
            };

        if (description.shader->stages.contains(ShaderStageBits::Geometry) ||
            description.shader->stages.contains(ShaderStageBits::TesselationControl) ||
            description.shader->stages.contains(ShaderStageBits::TesselationEvaluation))
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::UnsupportedUsage,
                    .message = "Graphics pipeline uses shader stages that are not currently supported",
                    .details = {
                        "Geometry and tessellation shader stages are not currently supported"
                    }
                }
            };

        if (auto compatibility = validateShaderLayoutCompatibility(*description.shader, *description.layout);
            !compatibility)
            return std::unexpected{std::move(compatibility).error()};

        if (auto state = validateGraphicsPipelineState(description); !state)
            return std::unexpected{std::move(state).error()};

        if (description.colorBlending.size() != description.colorFormats.size())
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = std::format(
                        "Color blending size {} must match color formats size {}",
                        description.colorBlending.size(),
                        description.colorFormats.size()
                    )
                }
            };

        if (description.colorFormats.size() > renderingDeviceDriver->getMaxColorAttachments())
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::ExceedsDeviceLimits,
                    .message = std::format(
                        "Color format count of {} exceeds device limits of {}",
                        description.colorFormats.size(),
                        renderingDeviceDriver->getMaxColorAttachments()
                    )
                }
            };

        for (size_t attachmentIndex = 0; attachmentIndex < description.colorFormats.size(); ++attachmentIndex) {
            const ImageDataFormat format = description.colorFormats[attachmentIndex];
            if (hasDepthAspect(format) || hasStencilAspect(format))
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::UnsupportedFormat,
                        .message = "Graphics pipeline color attachment uses a depth/stencil format",
                        .details = {
                            std::format("Color attachment: {}", attachmentIndex),
                            std::format("Format value: {}", static_cast<uint32_t>(format))
                        }
                    }
                };

            auto support = renderingDeviceDriver->validateAttachmentFormatSupport(
                format,
                ImageUsageBits::ColorAttachment,
                description.multisampling.samples
            );
            if (!support) {
                auto error = std::move(support).error();
                error.details.insert(
                    error.details.begin(),
                    std::format("Color attachment: {}", attachmentIndex)
                );
                return std::unexpected{std::move(error)};
            }
        }

        if (description.depthStencilFormat) {
            const ImageDataFormat format = *description.depthStencilFormat;
            if (!hasDepthAspect(format) && !hasStencilAspect(format))
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::UnsupportedFormat,
                        .message = "Graphics pipeline depth/stencil attachment uses a color-only format",
                        .details = {
                            std::format("Format value: {}", static_cast<uint32_t>(format))
                        }
                    }
                };

            auto support = renderingDeviceDriver->validateAttachmentFormatSupport(
                format,
                ImageUsageBits::DepthStencilAttachment,
                description.multisampling.samples
            );
            if (!support) {
                auto error = std::move(support).error();
                error.details.insert(error.details.begin(), "Depth/stencil attachment");
                return std::unexpected{std::move(error)};
            }
        }

        const auto makeLimitError = [](
            std::string message,
            std::string limit,
            const uint64_t requested,
            const uint64_t supported
        ) {
            return ResourceCreationError{
                .code = ResourceCreationErrorCode::ExceedsDeviceLimits,
                .message = std::move(message),
                .limitViolation = ResourceCreationLimitViolation{
                    .limit = std::move(limit),
                    .requested = requested,
                    .supported = supported
                }
            };
        };

        const uint32_t maxBindings = renderingDeviceDriver->getMaxVertexInputBindings();
        if (description.vertexBindings.size() > maxBindings)
            return std::unexpected{
                makeLimitError(
                    "Graphics pipeline contains too many vertex-input bindings",
                    "maxVertexInputBindings",
                    description.vertexBindings.size(),
                    maxBindings
                )
            };

        const uint32_t maxAttributes = renderingDeviceDriver->getMaxVertexInputAttributes();
        if (description.vertexAttributes.size() > maxAttributes)
            return std::unexpected{
                makeLimitError(
                    "Graphics pipeline contains too many vertex-input attributes",
                    "maxVertexInputAttributes",
                    description.vertexAttributes.size(),
                    maxAttributes
                )
            };

        const uint32_t maxStride = renderingDeviceDriver->getMaxVertexInputBindingStride();
        std::unordered_map<uint32_t, uint32_t> bindingStrides{};
        bindingStrides.reserve(description.vertexBindings.size());
        for (const auto& binding : description.vertexBindings) {
            if (binding.binding >= maxBindings)
                return std::unexpected{
                    makeLimitError(
                        std::format("Vertex-input binding {} exceeds the device limit", binding.binding),
                        "maxVertexInputBindings",
                        static_cast<uint64_t>(binding.binding) + 1,
                        maxBindings
                    )
                };

            if (binding.stride > maxStride)
                return std::unexpected{
                    makeLimitError(
                        std::format(
                            "Vertex-input binding {} has a stride that exceeds the device limit",
                            binding.binding
                        ),
                        "maxVertexInputBindingStride",
                        binding.stride,
                        maxStride
                    )
                };

            if (!bindingStrides.emplace(binding.binding, binding.stride).second)
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::InvalidDescription,
                        .message = "Graphics pipeline contains a duplicate vertex-input binding",
                        .details = {std::format("Binding: {}", binding.binding)}
                    }
                };
        }

        const uint32_t maxAttributeOffset = renderingDeviceDriver->getMaxVertexInputAttributeOffset();
        std::unordered_set<uint32_t> attributeLocations{};
        attributeLocations.reserve(description.vertexAttributes.size());
        for (const auto& attribute : description.vertexAttributes) {
            if (attribute.location >= maxAttributes)
                return std::unexpected{
                    makeLimitError(
                        std::format("Vertex-input attribute location {} exceeds the device limit", attribute.location),
                        "maxVertexInputAttributes",
                        static_cast<uint64_t>(attribute.location) + 1,
                        maxAttributes
                    )
                };

            if (!attributeLocations.insert(attribute.location).second)
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::InvalidDescription,
                        .message = "Graphics pipeline contains a duplicate vertex-input attribute location",
                        .details = {std::format("Location: {}", attribute.location)}
                    }
                };

            const auto binding = bindingStrides.find(attribute.binding);
            if (binding == bindingStrides.end())
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::InvalidDescription,
                        .message = "Vertex-input attribute references an undeclared binding",
                        .details = {
                            std::format("Location: {}; binding: {}", attribute.location, attribute.binding)
                        }
                    }
                };

            const uint32_t formatSize = getTexelSize(attribute.format);
            if (formatSize == 0 || hasDepthAspect(attribute.format) || hasStencilAspect(attribute.format))
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::UnsupportedFormat,
                        .message = "Vertex-input attribute uses a format that is not suitable for vertex data",
                        .details = {std::format("Location: {}", attribute.location)}
                    }
                };

            if (!renderingDeviceDriver->isVertexInputFormatSupported(attribute.format))
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::UnsupportedFormat,
                        .message = "Vertex-input attribute format is not supported by the rendering device",
                        .details = {std::format("Location: {}", attribute.location)}
                    }
                };

            if (attribute.offset > maxAttributeOffset)
                return std::unexpected{
                    makeLimitError(
                        std::format(
                            "Vertex-input attribute location {} has an offset that exceeds the device limit",
                            attribute.location
                        ),
                        "maxVertexInputAttributeOffset",
                        attribute.offset,
                        maxAttributeOffset
                    )
                };

            const uint64_t attributeEnd = static_cast<uint64_t>(attribute.offset) + formatSize;
            if (binding->second != 0 && attributeEnd > binding->second)
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::InvalidDescription,
                        .message = "Vertex-input attribute extends beyond its binding stride",
                        .details = {
                            std::format(
                                "Location: {}; binding: {}; attribute byte range: [{}, {}); stride: {}",
                                attribute.location,
                                attribute.binding,
                                attribute.offset,
                                attributeEnd,
                                binding->second
                            )
                        }
                    }
                };
        }

        const auto pipeline = renderingDeviceDriver->createGraphicsPipeline(description);
        if (!pipeline)
            return std::unexpected{std::move(pipeline).error()};

        if (!*pipeline)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::NativeObjectCreationFailed,
                    .message = "The rendering backend reported successful pipeline creation but returned a null pointer"
                }
            };

        return *pipeline;
    } catch (const std::bad_alloc&) {
        return std::unexpected{
            ResourceCreationError{
                .code = ResourceCreationErrorCode::OutOfHostMemory,
                .message = "Failed to allocate temporary storage while creating a graphics pipeline"
            }
        };
    }

    auto RenderingDevice::createComputePipeline(
        const ComputePipelineDescription& description
    ) const -> std::expected<ComputePipeline*, ResourceCreationError> try {
        if (description.shader == nullptr)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = "Compute pipeline description does not specify a shader"
                }
            };

        if (description.layout == nullptr)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = "Compute pipeline description does not specify a pipeline layout"
                }
            };

        if (description.shader->stages != ShaderStageFlags{ShaderStageBits::Compute})
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = "Compute pipeline shader must contain exactly one compute stage and no graphics stages"
                }
            };

        if (auto compatibility = validateShaderLayoutCompatibility(*description.shader, *description.layout);
            !compatibility)
            return std::unexpected{std::move(compatibility).error()};

        const auto pipeline = renderingDeviceDriver->createComputePipeline(description);
        if (!pipeline)
            return std::unexpected{std::move(pipeline).error()};

        if (!*pipeline)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::NativeObjectCreationFailed,
                    .message = "The rendering backend reported successful pipeline creation but returned a null pointer"
                }
            };

        return *pipeline;
    } catch (const std::bad_alloc&) {
        return std::unexpected{
            ResourceCreationError{
                .code = ResourceCreationErrorCode::OutOfHostMemory,
                .message = "Failed to allocate temporary storage while creating a compute pipeline"
            }
        };
    }

    RenderingContextDriver* RenderingDevice::getRenderingContextDriver() const {
        return renderingContextDriver;
    }

    RenderingDeviceDriver* RenderingDevice::getRenderingDeviceDriver() const {
        return renderingDeviceDriver;
    }
}
