#pragma once

#include <cstdint>
#include <variant>

#include "buffer/Buffer.h"
#include "image/Image.h"

namespace Vixen {
    enum class ResourceLifetime;
    enum class ResourceType;

    using ResourceObject = std::variant<std::monostate, Image*, Buffer*>;

    enum class Ownership {
        Empty,
        Imported,
        Owned
    };

    struct FrameGraphResourceSlot {
        ResourceType type;

        ResourceLifetime lifetime;

        uint32_t latestVersion = 0;

        ResourceObject object = std::monostate{};

        Ownership ownership = Ownership::Empty;
    };
}
