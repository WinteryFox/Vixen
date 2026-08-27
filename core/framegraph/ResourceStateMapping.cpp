#include "ResourceStateMapping.h"

namespace Vixen {
    namespace {
        constexpr auto shaderStages = PipelineStageBits::VertexShader |
            PipelineStageBits::TessellationControl |
            PipelineStageBits::TessellationEvaluation |
            PipelineStageBits::GeometryShader |
            PipelineStageBits::FragmentShader |
            PipelineStageBits::ComputeShader;

        constexpr auto depthStages = PipelineStageBits::EarlyFragmentTests |
            PipelineStageBits::LateFragmentTests;

        [[nodiscard]] bool isSubset(
            const PipelineStageFlags value,
            const PipelineStageFlags allowed
        ) {
            return (value.value() & ~allowed.value()) == 0;
        }

        auto requireStages(
            const PipelineStageFlags stages,
            const PipelineStageFlags allowed
        ) -> std::expected<void, FrameGraphError> {
            if (stages.empty())
                return std::unexpected(FrameGraphError{
                    .code = FrameGraphErrorCode::MissingPipelineStages,
                    .message = "A resource use must declare at least one pipeline stage."
                });

            if (!isSubset(stages, allowed))
                return std::unexpected(FrameGraphError{
                    .code = FrameGraphErrorCode::IncompatiblePipelineStages,
                    .message = "The declared pipeline stages are incompatible with the resource usage."
                });

            return {};
        }

        auto requireAccess(
            const ResourceAccess actual,
            const ResourceAccess required
        ) -> std::expected<void, FrameGraphError> {
            if (actual != required)
                return std::unexpected(FrameGraphError{
                    .code = FrameGraphErrorCode::InvalidAccess,
                    .message = "The declared read/write access is incompatible with the resource usage."
                });

            return {};
        }

        BarrierAccessFlags shaderAccess(const ResourceAccess access) {
            switch (access) {
                case ResourceAccess::Read:
                    return BarrierAccessBits::ShaderRead;

                case ResourceAccess::Write:
                    return BarrierAccessBits::ShaderWrite;

                case ResourceAccess::ReadWrite:
                    return BarrierAccessBits::ShaderRead | BarrierAccessBits::ShaderWrite;
            }

            return {};
        }

        BarrierAccessFlags attachmentAccess(
            const ResourceAccess access,
            const BarrierAccessBits read,
            const BarrierAccessBits write
        ) {
            switch (access) {
                case ResourceAccess::Read:
                    return read;

                case ResourceAccess::Write:
                    return write;

                case ResourceAccess::ReadWrite:
                    return read | write;
            }
            return {};
        }
    }

    auto mapImageResourceState(
        const ResourceAccess access,
        const ImageUsageBits usage,
        const PipelineStageFlags stages
    ) -> std::expected<ImageState, FrameGraphError> {
        switch (usage) {
            case ImageUsageBits::Sampling:
                if (auto result = requireAccess(access, ResourceAccess::Read); !result)
                    return std::unexpected(result.error());

                if (auto result = requireStages(stages, shaderStages); !result)
                    return std::unexpected(result.error());

                return ImageState{
                    .stages = stages,
                    .access = BarrierAccessBits::ShaderRead,
                    .layout = ImageLayout::ShaderReadOnlyOptimal
                };

            case ImageUsageBits::Storage:
                if (auto result = requireStages(stages, shaderStages); !result)
                    return std::unexpected(result.error());

                return ImageState{
                    .stages = stages,
                    .access = shaderAccess(access),
                    .layout = ImageLayout::StorageOptimal
                };

            case ImageUsageBits::StorageAtomic:
                if (auto result = requireAccess(access, ResourceAccess::ReadWrite); !result)
                    return std::unexpected(result.error());

                if (auto result = requireStages(stages, shaderStages); !result)
                    return std::unexpected(result.error());

                return ImageState{
                    .stages = stages,
                    .access = shaderAccess(access),
                    .layout = ImageLayout::StorageOptimal
                };

            case ImageUsageBits::ColorAttachment:
                if (auto result = requireStages(stages, PipelineStageBits::ColorAttachmentOutput); !result)
                    return std::unexpected(result.error());

                return ImageState{
                    .stages = stages,
                    .access = attachmentAccess(access, BarrierAccessBits::ColorAttachmentRead,
                                               BarrierAccessBits::ColorAttachmentWrite),
                    .layout = ImageLayout::ColorAttachmentOptimal
                };

            case ImageUsageBits::DepthStencilAttachment:
                if (auto result = requireStages(stages, depthStages); !result)
                    return std::unexpected(result.error());

                return ImageState{
                    .stages = stages,
                    .access = attachmentAccess(access, BarrierAccessBits::DepthStencilAttachmentRead,
                                               BarrierAccessBits::DepthStencilAttachmentWrite),
                    .layout = access == ResourceAccess::Read
                                  ? ImageLayout::DepthStencilReadOnlyOptimal
                                  : ImageLayout::DepthStencilAttachmentOptimal
                };

            case ImageUsageBits::CopySource:
                if (auto result = requireAccess(access, ResourceAccess::Read); !result)
                    return std::unexpected(result.error());

                if (auto result = requireStages(stages, PipelineStageBits::Copy); !result)
                    return std::unexpected(result.error());

                return ImageState{
                    .stages = stages,
                    .access = BarrierAccessBits::CopyRead,
                    .layout = ImageLayout::CopySourceOptimal
                };

            case ImageUsageBits::CopyDestination:
                if (auto result = requireAccess(access, ResourceAccess::Write); !result)
                    return std::unexpected(result.error());

                if (auto result = requireStages(stages, PipelineStageBits::Copy); !result)
                    return std::unexpected(result.error());

                return ImageState{
                    .stages = stages,
                    .access = BarrierAccessBits::CopyWrite,
                    .layout = ImageLayout::CopyDestinationOptimal
                };

            case ImageUsageBits::InputAttachment:
                if (auto result = requireAccess(access, ResourceAccess::Read); !result)
                    return std::unexpected(result.error());

                if (auto result = requireStages(stages, PipelineStageBits::FragmentShader); !result)
                    return std::unexpected(result.error());

                return ImageState{
                    .stages = stages,
                    .access = BarrierAccessBits::InputAttachmentRead,
                    .layout = ImageLayout::ShaderReadOnlyOptimal
                };

            case ImageUsageBits::CpuRead:
            case ImageUsageBits::Update:
            case ImageUsageBits::TransientAttachment:
                return std::unexpected(FrameGraphError{
                    .code = FrameGraphErrorCode::UnsupportedUsage,
                    .message = "This image usage is a creation or host-operation capability, not a pass access."
                });
        }

        return std::unexpected(FrameGraphError{FrameGraphErrorCode::UnsupportedUsage, "Unknown image usage."});
    }

    auto mapBufferResourceState(
        const ResourceAccess access,
        const BufferUsageBits usage,
        const PipelineStageFlags stages
    ) -> std::expected<BufferState, FrameGraphError> {
        switch (usage) {
            case BufferUsageBits::CopySource:
                if (auto result = requireAccess(access, ResourceAccess::Read); !result)
                    return std::unexpected(result.error());

                if (auto result = requireStages(stages, PipelineStageBits::Copy); !result)
                    return std::unexpected(result.error());

                return BufferState{
                    .stages = stages,
                    .access = BarrierAccessBits::CopyRead
                };

            case BufferUsageBits::CopyDestination:
                if (auto result = requireAccess(access, ResourceAccess::Write); !result)
                    return std::unexpected(result.error());

                if (auto result = requireStages(stages, PipelineStageBits::Copy); !result)
                    return std::unexpected(result.error());

                return BufferState{
                    .stages = stages,
                    .access = BarrierAccessBits::CopyWrite
                };

            case BufferUsageBits::Uniform:
                if (auto result = requireAccess(access, ResourceAccess::Read); !result)
                    return std::unexpected(result.error());

                if (auto result = requireStages(stages, shaderStages); !result)
                    return std::unexpected(result.error());

                return BufferState{
                    .stages = stages,
                    .access = BarrierAccessBits::UniformRead
                };

            case BufferUsageBits::Storage:
            case BufferUsageBits::Texel:
                if (auto result = requireStages(stages, shaderStages); !result)
                    return std::unexpected(result.error());

                return BufferState{
                    .stages = stages,
                    .access = shaderAccess(access)
                };

            case BufferUsageBits::Vertex:
                if (auto result = requireAccess(access, ResourceAccess::Read); !result)
                    return std::unexpected(result.error());

                if (auto result = requireStages(stages, PipelineStageBits::VertexInput); !result)
                    return std::unexpected(result.error());

                return BufferState{
                    .stages = stages,
                    .access = BarrierAccessBits::VertexAttributeRead
                };

            case BufferUsageBits::Index:
                if (auto result = requireAccess(access, ResourceAccess::Read); !result)
                    return std::unexpected(result.error());

                if (auto result = requireStages(stages, PipelineStageBits::VertexInput); !result)
                    return std::unexpected(result.error());

                return BufferState{
                    .stages = stages,
                    .access = BarrierAccessBits::IndexRead
                };

            case BufferUsageBits::Indirect:
                if (auto result = requireAccess(access, ResourceAccess::Read); !result)
                    return std::unexpected(result.error());

                if (auto result = requireStages(stages, PipelineStageBits::DrawIndirect); !result)
                    return std::unexpected(result.error());

                return BufferState{
                    .stages = stages,
                    .access = BarrierAccessBits::IndirectCommandsRead
                };
        }

        return std::unexpected(FrameGraphError{
            .code = FrameGraphErrorCode::UnsupportedUsage,
            .message = "Unknown buffer usage."
        });
    }

    auto validateImageUsage(
        const ImageUsageFlags declared,
        const ImageUsageBits requested
    ) -> std::expected<void, FrameGraphError> {
        if (!declared.contains(requested))
            return std::unexpected(FrameGraphError{
                .code = FrameGraphErrorCode::UsageNotDeclared,
                .message = "The image was not created with the requested usage."
            });

        return {};
    }

    auto validateBufferUsage(
        const BufferUsageFlags declared,
        const BufferUsageBits requested
    ) -> std::expected<void, FrameGraphError> {
        if (!declared.contains(requested))
            return std::unexpected(FrameGraphError{
                .code = FrameGraphErrorCode::UsageNotDeclared,
                .message = "The buffer was not created with the requested usage."
            });
        return {};
    }
}
