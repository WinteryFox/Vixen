#pragma once

#include <compare>
#include <cstdint>
#include <limits>
#include <variant>

#include "core/synchronization/BarrierAccessFlags.h"
#include "core/synchronization/PipelineStageFlags.h"
#include "core/buffer/BufferUsage.h"
#include "core/image/ImageLayout.h"
#include "core/image/ImageUsage.h"

namespace Vixen {
    enum class ResourceType {
        Image,
        Buffer
    };

    enum class ResourceAccess {
        Read,
        Write,
        ReadWrite
    };

    enum class ResourceLifetime {
        Transient,
        Persistent,
        Imported
    };

    struct ResourceId {
        /**
         * Versions are linear declaration-order tokens. Reads retain the current
         * version, while writes and read-writes produce the next version. Only the
         * latest version may be declared by a later pass, and all versions of a
         * logical resource resolve to the same physical resource.
         */
        static constexpr uint32_t Invalid =
            std::numeric_limits<uint32_t>::max();

        uint32_t index = Invalid;
        uint32_t version = 0;

        [[nodiscard]]
        constexpr bool isValid() const noexcept {
            return index != Invalid;
        }

        std::strong_ordering operator<=>(const ResourceId&) const = default;
    };

    template <typename T>
    struct ResourceHandle {
        ResourceId id;

        [[nodiscard]]
        constexpr bool isValid() const noexcept {
            return id.isValid();
        }

        auto operator<=>(const ResourceHandle&) const = default;
    };

    struct ImageTag {};

    struct BufferTag {};

    using ImageHandle = ResourceHandle<ImageTag>;
    using BufferHandle = ResourceHandle<BufferTag>;

    struct ImageResourceUsage {
        ImageHandle input;
        ImageHandle output;

        ResourceAccess access;

        ImageUsageBits usage;

        PipelineStageFlags stages;
    };

    struct BufferResourceUsage {
        BufferHandle input;
        BufferHandle output;

        ResourceAccess access;

        BufferUsageBits usage;

        PipelineStageFlags stages;
    };

    using ResourceUsage = std::variant<ImageResourceUsage, BufferResourceUsage>;

    struct ImageState {
        PipelineStageFlags stages{};

        BarrierAccessFlags access{};

        ImageLayout layout = ImageLayout::Undefined;
    };

    struct BufferState {
        PipelineStageFlags stages{};

        BarrierAccessFlags access{};
    };
}
