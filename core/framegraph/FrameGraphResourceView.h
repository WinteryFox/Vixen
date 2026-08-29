#pragma once

#include <expected>
#include <format>
#include <span>
#include <variant>

#include "FrameGraphError.h"
#include "Resource.h"
#include "ResourceSlot.h"

namespace Vixen {
    class Buffer;
    struct Image;
    struct FrameGraphResourceSlot;

    class FrameGraphResourceView {
        std::span<const FrameGraphResourceSlot> resources;

        [[nodiscard]] auto get(
            const ResourceId id,
            const ResourceType type
        ) const -> std::expected<ResourceObject, FrameGraphError> {
            if (!id.isValid())
                return std::unexpected{
                    FrameGraphError{
                        .code = FrameGraphErrorCode::InvalidResourceHandle,
                        .message = "A default initialized resource handle is not a valid lookup handle",
                    }
                };

            if (id.index >= resources.size())
                return std::unexpected{
                    FrameGraphError{
                        .code = FrameGraphErrorCode::InvalidResourceHandle,
                        .message = std::format(
                            "Handle index {} is out of bounds ({})",
                            id.index,
                            resources.size()
                        ),
                        .resourceIndex = id.index,
                        .resourceVersion = id.version
                    }
                };

            const auto& resource = resources[id.index];

            if (id.version > resource.latestVersion)
                return std::unexpected{
                    FrameGraphError{
                        .code = FrameGraphErrorCode::InvalidResourceVersion,
                        .message = std::format(
                            "Resource version {} exceeds recorded latest version {} for resource with index {}",
                            id.version,
                            resource.latestVersion,
                            id.index
                        ),
                        .resourceIndex = id.index,
                        .resourceVersion = id.version
                    }
                };

            if (resource.type != type)
                return std::unexpected{
                    FrameGraphError{
                        .code = FrameGraphErrorCode::ResourceTypeMismatch,
                        .message = std::format(
                            "Resource {} was expected to be of type {} but is actually {}",
                            id.index,
                            type == ResourceType::Image ? "ResourceType::Image" : "ResourceType::Buffer",
                            resource.type == ResourceType::Image ? "ResourceType::Image" : "ResourceType::Buffer"
                        ),
                        .resourceIndex = id.index,
                        .resourceVersion = id.version
                    }
                };

            if (resource.ownership == Ownership::Empty)
                return std::unexpected{
                    FrameGraphError{
                        .code = FrameGraphErrorCode::UnresolvedResource,
                        .message = std::format(
                            "Resource {} has empty ownership",
                            id.index
                        ),
                        .resourceIndex = id.index,
                        .resourceVersion = id.version
                    }
                };

            if (holds_alternative<std::monostate>(resource.object))
                return std::unexpected{
                    FrameGraphError{
                        .code = FrameGraphErrorCode::UnresolvedResource,
                        .message = std::format(
                            "Resource {} is uninitialized",
                            id.index
                        ),
                        .resourceIndex = id.index,
                        .resourceVersion = id.version
                    }
                };

            return resource.object;
        }

    public:
        explicit FrameGraphResourceView(
            const std::span<const FrameGraphResourceSlot> resources) : resources(resources) {}

        [[nodiscard]] auto get(const ImageHandle handle) const -> std::expected<Image*, FrameGraphError> {
            const auto resource = get(handle.id, ResourceType::Image);
            if (!resource)
                return std::unexpected{std::move(resource).error()};

            const auto pointer = std::get_if<Image*>(&*resource);

            if (pointer == nullptr)
                return std::unexpected{
                    FrameGraphError{
                        .code = FrameGraphErrorCode::InvalidGraphInvariant,
                        .message = std::format(
                            "Resource slot {} is declared as an image but stores a different object type",
                            handle.id.index
                        ),
                        .resourceIndex = handle.id.index,
                        .resourceVersion = handle.id.version
                    }
                };

            if (*pointer == nullptr)
                return std::unexpected{
                    FrameGraphError{
                        .code = FrameGraphErrorCode::UnresolvedResource,
                        .message = std::format(
                            "Image resource {} resolves to a null pointer",
                            handle.id.index
                        ),
                        .resourceIndex = handle.id.index,
                        .resourceVersion = handle.id.version
                    }
                };

            return *pointer;
        }

        [[nodiscard]] auto get(const BufferHandle handle) const -> std::expected<Buffer*, FrameGraphError> {
            const auto resource = get(handle.id, ResourceType::Buffer);
            if (!resource)
                return std::unexpected{std::move(resource).error()};

            const auto pointer = std::get_if<Buffer*>(&*resource);

            if (pointer == nullptr)
                return std::unexpected{
                    FrameGraphError{
                        .code = FrameGraphErrorCode::InvalidGraphInvariant,
                        .message = std::format(
                            "Resource slot {} is declared as a buffer but stores a different object type",
                            handle.id.index
                        ),
                        .resourceIndex = handle.id.index,
                        .resourceVersion = handle.id.version
                    }
                };

            if (*pointer == nullptr)
                return std::unexpected{
                    FrameGraphError{
                        .code = FrameGraphErrorCode::UnresolvedResource,
                        .message = std::format(
                            "Buffer resource {} resolves to a null pointer",
                            handle.id.index
                        ),
                        .resourceIndex = handle.id.index,
                        .resourceVersion = handle.id.version
                    }
                };

            return *pointer;
        }
    };
}
