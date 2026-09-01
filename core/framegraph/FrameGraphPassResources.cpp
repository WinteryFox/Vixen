#include "FrameGraphPassResources.h"

#include <format>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>

#include "FrameGraphError.h"
#include "Node.h"
#include "buffer/Buffer.h"
#include "image/Image.h"

namespace Vixen {
    namespace {
        [[nodiscard]] constexpr bool bufferRangeWithinBounds(
            const std::uint64_t bufferSize,
            const std::uint64_t offset,
            const std::optional<std::uint64_t> size
        ) noexcept {
            return offset <= bufferSize &&
                (!size.has_value() || *size <= bufferSize - offset);
        }

        static_assert(bufferRangeWithinBounds(64, 0, std::nullopt));
        static_assert(bufferRangeWithinBounds(64, 16, 48));
        static_assert(bufferRangeWithinBounds(64, 64, 0));
        static_assert(!bufferRangeWithinBounds(64, 65, std::nullopt));
        static_assert(!bufferRangeWithinBounds(64, 32, 33));
        static_assert(!bufferRangeWithinBounds(
            std::numeric_limits<std::uint64_t>::max(),
            std::numeric_limits<std::uint64_t>::max() - 1,
            3
        ));

        [[nodiscard]] FrameGraphResourceAccessErrorCode translateLookupError(
            const FrameGraphErrorCode code
        ) noexcept {
            switch (code) {
                case FrameGraphErrorCode::InvalidResourceHandle:
                    return FrameGraphResourceAccessErrorCode::InvalidHandle;
                case FrameGraphErrorCode::InvalidResourceVersion:
                    return FrameGraphResourceAccessErrorCode::UndeclaredVersion;
                case FrameGraphErrorCode::ResourceTypeMismatch:
                    return FrameGraphResourceAccessErrorCode::ResourceTypeMismatch;
                case FrameGraphErrorCode::UnresolvedResource:
                    return FrameGraphResourceAccessErrorCode::UnresolvedResource;
                case FrameGraphErrorCode::UnsupportedUsage:
                case FrameGraphErrorCode::InvalidAccess:
                case FrameGraphErrorCode::MissingPipelineStages:
                case FrameGraphErrorCode::IncompatiblePipelineStages:
                case FrameGraphErrorCode::UsageNotDeclared:
                case FrameGraphErrorCode::ResourceVersionOverflow:
                case FrameGraphErrorCode::DuplicateProducer:
                case FrameGraphErrorCode::InvalidGraphInvariant:
                case FrameGraphErrorCode::IncompatiblePassStages:
                case FrameGraphErrorCode::InvalidResourceDeclaration:
                case FrameGraphErrorCode::InvalidResourceOwnership:
                case FrameGraphErrorCode::InvalidUsageShape:
                case FrameGraphErrorCode::UninitializedResourceRead:
                case FrameGraphErrorCode::MissingProducer:
                case FrameGraphErrorCode::DependencyCycle:
                case FrameGraphErrorCode::InvalidAttachment:
                case FrameGraphErrorCode::ResourceAllocationFailed:
                    return FrameGraphResourceAccessErrorCode::InvalidInvariant;
            }

            return FrameGraphResourceAccessErrorCode::InvalidInvariant;
        }
    }

    FrameGraphPassResources::FrameGraphPassResources(
        const FrameGraphResourceView resources,
        const std::span<const FrameGraphResourcePermission> permissions,
        const std::span<const ResourceNode> nodes,
        const std::uint32_t passIndex,
        const std::string_view passName,
        const bool sideEffecting,
        const bool usesExternallySynchronizedResources
    ) noexcept : resources(resources),
                 permissions(permissions),
                 nodes(nodes),
                 passIndex(passIndex),
                 passName(passName),
                 sideEffecting(sideEffecting),
                 usesExternallySynchronizedResources(usesExternallySynchronizedResources) {}

    auto FrameGraphPassResources::authorize(
        const FrameGraphResourceAccessRequest& request
    ) const -> std::expected<const FrameGraphResourcePermission*, FrameGraphResourceAccessError> {
        const auto resourceName = request.handle.isValid() && request.handle.index < nodes.size()
                                      ? std::string_view{nodes[request.handle.index].name}
                                      : std::string_view{};

        return authorizeFrameGraphResourceAccess(
            permissions,
            request,
            FrameGraphResourceAccessContext{
                .passIndex = passIndex,
                .passName = passName,
                .resourceName = resourceName,
                .sideEffecting = sideEffecting,
                .usesExternallySynchronizedResources = usesExternallySynchronizedResources
            }
        );
    }

    auto FrameGraphPassResources::resolveImage(
        const ImageHandle handle,
        const ImageUsageBits usage,
        const ResourceAccess access
    ) const -> std::expected<Image*, FrameGraphResourceAccessError> {
        const FrameGraphResourceAccessRequest request{
            .handle = handle.id,
            .type = ResourceType::Image,
            .access = access,
            .usage = FrameGraphResourceUsageKind{usage}
        };

        auto permission = authorize(request);
        if (!permission)
            return std::unexpected{std::move(permission).error()};

        auto image = resources.get(handle);
        if (!image) {
            auto lookupError = std::move(image).error();
            FrameGraphResourceAccessError error{
                .code = translateLookupError(lookupError.code),
                .message = std::format(
                    "Authorized image resource index {} version {} could not be resolved: {}",
                    handle.id.index,
                    handle.id.version,
                    lookupError.message
                ),
                .passIndex = passIndex,
                .resourceIndex = handle.id.index,
                .resourceVersion = handle.id.version,
                .passName = std::string{passName},
                .requestedOperation = request,
                .declaredOperations = {**permission},
                .details = std::move(lookupError.details)
            };

            if (handle.id.index < nodes.size())
                error.resourceName = nodes[handle.id.index].name;

            return std::unexpected{std::move(error)};
        }

        return *image;
    }

    auto FrameGraphPassResources::resolveBuffer(
        const BufferHandle handle,
        const BufferUsageBits usage,
        const ResourceAccess access,
        const std::uint64_t offset,
        const std::optional<std::uint64_t> size
    ) const -> std::expected<Buffer*, FrameGraphResourceAccessError> {
        const FrameGraphResourceAccessRequest request{
            .handle = handle.id,
            .type = ResourceType::Buffer,
            .access = access,
            .usage = FrameGraphResourceUsageKind{usage}
        };

        auto permission = authorize(request);
        if (!permission)
            return std::unexpected{std::move(permission).error()};

        auto buffer = resources.get(handle);
        if (!buffer) {
            auto lookupError = std::move(buffer).error();
            FrameGraphResourceAccessError error{
                .code = translateLookupError(lookupError.code),
                .message = std::format(
                    "Authorized buffer resource index {} version {} could not be resolved: {}",
                    handle.id.index,
                    handle.id.version,
                    lookupError.message
                ),
                .passIndex = passIndex,
                .resourceIndex = handle.id.index,
                .resourceVersion = handle.id.version,
                .passName = std::string{passName},
                .requestedOperation = request,
                .declaredOperations = {**permission},
                .details = std::move(lookupError.details)
            };

            if (handle.id.index < nodes.size())
                error.resourceName = nodes[handle.id.index].name;

            return std::unexpected{std::move(error)};
        }

        const auto bufferSize = (*buffer)->getSize();
        if (!bufferRangeWithinBounds(bufferSize, offset, size)) {
            FrameGraphResourceAccessError error{
                .code = FrameGraphResourceAccessErrorCode::RangeOutOfBounds,
                .message = size.has_value()
                               ? std::format(
                                   "Requested buffer range (offset {}, size {}) exceeds resource '{}' size {}",
                                   offset,
                                   *size,
                                   handle.id.index < nodes.size()
                                       ? nodes[handle.id.index].name
                                       : std::to_string(handle.id.index),
                                   bufferSize
                               )
                               : std::format(
                                   "Requested buffer offset {} exceeds resource '{}' size {}",
                                   offset,
                                   handle.id.index < nodes.size()
                                       ? nodes[handle.id.index].name
                                       : std::to_string(handle.id.index),
                                   bufferSize
                               ),
                .passIndex = passIndex,
                .resourceIndex = handle.id.index,
                .resourceVersion = handle.id.version,
                .passName = std::string{passName},
                .requestedOperation = request,
                .declaredOperations = {**permission}
            };

            if (handle.id.index < nodes.size())
                error.resourceName = nodes[handle.id.index].name;

            error.details.push_back(
                "Buffer ranges are half-open; omitted size means the remainder of the buffer"
            );

            return std::unexpected{std::move(error)};
        }

        return *buffer;
    }

    auto FrameGraphPassResources::readImage(
        const ImageHandle handle,
        const ImageUsageBits usage
    ) const -> std::expected<const Image*, FrameGraphResourceAccessError> {
        auto image = resolveImage(handle, usage, ResourceAccess::Read);
        if (!image)
            return std::unexpected{std::move(image).error()};
        return *image;
    }

    auto FrameGraphPassResources::writeImage(
        const ImageHandle handle,
        const ImageUsageBits usage
    ) const -> std::expected<Image*, FrameGraphResourceAccessError> {
        return resolveImage(handle, usage, ResourceAccess::Write);
    }

    auto FrameGraphPassResources::readBuffer(
        const BufferHandle handle,
        const BufferUsageBits usage,
        const std::uint64_t offset,
        const std::optional<std::uint64_t> size
    ) const -> std::expected<const Buffer*, FrameGraphResourceAccessError> {
        auto buffer = resolveBuffer(handle, usage, ResourceAccess::Read, offset, size);
        if (!buffer)
            return std::unexpected{std::move(buffer).error()};
        return *buffer;
    }

    auto FrameGraphPassResources::writeBuffer(
        const BufferHandle handle,
        const BufferUsageBits usage,
        const std::uint64_t offset,
        const std::optional<std::uint64_t> size
    ) const -> std::expected<Buffer*, FrameGraphResourceAccessError> {
        return resolveBuffer(handle, usage, ResourceAccess::Write, offset, size);
    }

    static_assert(!std::is_copy_constructible_v<FrameGraphPassResources>);
    static_assert(!std::is_copy_assignable_v<FrameGraphPassResources>);
    static_assert(!std::is_move_constructible_v<FrameGraphPassResources>);
    static_assert(!std::is_move_assignable_v<FrameGraphPassResources>);
}
