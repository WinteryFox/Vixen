#pragma once

#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <string_view>

#include "FrameGraphResourceAccess.h"
#include "FrameGraphResourceView.h"

namespace Vixen {
    class Buffer;
    class FrameGraph;
    struct Image;
    struct ResourceNode;

    /**
     * @brief A pass-scoped authorization view over physical frame-graph resources.
     *
     * The resolver is created immediately before one pass callback and destroyed
     * immediately afterwards. It intentionally cannot be copied or moved. A
     * callback may resolve only the exact logical versions, access modes, and
     * usages declared by that pass, even though all versions of a logical
     * resource share one physical object.
     *
     * Image access covers the complete image view in version one. Buffer access
     * may optionally validate a byte range. Resource access checks remain enabled
     * in release builds.
     */
    class FrameGraphPassResources final {
        FrameGraphResourceView resources;
        std::span<const FrameGraphResourcePermission> permissions;
        std::span<const ResourceNode> nodes;
        std::uint32_t passIndex;
        std::string_view passName;
        bool sideEffecting;
        bool usesExternallySynchronizedResources;

        FrameGraphPassResources(
            FrameGraphResourceView resources,
            std::span<const FrameGraphResourcePermission> permissions,
            std::span<const ResourceNode> nodes,
            std::uint32_t passIndex,
            std::string_view passName,
            bool sideEffecting,
            bool usesExternallySynchronizedResources
        ) noexcept;

        [[nodiscard]] auto authorize(
            const FrameGraphResourceAccessRequest& request
        ) const -> std::expected<const FrameGraphResourcePermission*, FrameGraphResourceAccessError>;

        [[nodiscard]] auto resolveImage(
            ImageHandle handle,
            ImageUsageBits usage,
            ResourceAccess access
        ) const -> std::expected<Image*, FrameGraphResourceAccessError>;

        [[nodiscard]] auto resolveBuffer(
            BufferHandle handle,
            BufferUsageBits usage,
            ResourceAccess access,
            std::uint64_t offset,
            std::optional<std::uint64_t> size
        ) const -> std::expected<Buffer*, FrameGraphResourceAccessError>;

        friend class FrameGraph;

    public:
        FrameGraphPassResources(const FrameGraphPassResources&) = delete;
        FrameGraphPassResources& operator=(const FrameGraphPassResources&) = delete;
        FrameGraphPassResources(FrameGraphPassResources&&) = delete;
        FrameGraphPassResources& operator=(FrameGraphPassResources&&) = delete;
        ~FrameGraphPassResources() = default;

        /** Resolves a declared image version for read-only access. */
        [[nodiscard]] auto readImage(
            ImageHandle handle,
            ImageUsageBits usage
        ) const -> std::expected<const Image*, FrameGraphResourceAccessError>;

        /** Resolves a declared output image version for writable access. */
        [[nodiscard]] auto writeImage(
            ImageHandle handle,
            ImageUsageBits usage
        ) const -> std::expected<Image*, FrameGraphResourceAccessError>;

        /**
         * Resolves a declared buffer version for read-only access and validates
         * the optional half-open byte range [offset, offset + size).
         * An omitted size covers the remainder of the buffer.
         */
        [[nodiscard]] auto readBuffer(
            BufferHandle handle,
            BufferUsageBits usage,
            std::uint64_t offset = 0,
            std::optional<std::uint64_t> size = std::nullopt
        ) const -> std::expected<const Buffer*, FrameGraphResourceAccessError>;

        /**
         * Resolves a declared output buffer version for writable access and
         * validates the optional half-open byte range [offset, offset + size).
         * An omitted size covers the remainder of the buffer.
         */
        [[nodiscard]] auto writeBuffer(
            BufferHandle handle,
            BufferUsageBits usage,
            std::uint64_t offset = 0,
            std::optional<std::uint64_t> size = std::nullopt
        ) const -> std::expected<Buffer*, FrameGraphResourceAccessError>;
    };
}
