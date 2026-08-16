#pragma once

#include <compare>
#include <cstdint>
#include <limits>

#include "PipelineStageFlags.h"

namespace Vixen {
    enum class ResourceType {
        Texture,
        Buffer
    };

    enum class ResourceAccess {
        Read,
        Write,
        ReadWrite
    };

    enum class ResourceLifetime {
        Transient,
        Persistent
    };

    struct ResourceId {
        static constexpr uint32_t Invalid =
            std::numeric_limits<uint32_t>::max();

        uint32_t index = Invalid;

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

    struct ResourceUsage {
        ResourceId resource;

        ResourceAccess access;

        ResourceType type;

        PipelineStageBits stage;
    };
}
