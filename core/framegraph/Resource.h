#pragma once

#include <compare>
#include <cstdint>
#include <limits>

#include "../BarrierAccessFlags.h"
#include "../PipelineStageFlags.h"
#include "../buffer/BufferUsage.h"
#include "../image/ImageLayout.h"
#include "../image/ImageUsage.h"

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
        ImageHandle handle;

        ResourceAccess access;

        ImageUsageBits usage;

        PipelineStageFlags stages;
    };

    struct BufferResourceUsage {
        BufferHandle handle;

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
