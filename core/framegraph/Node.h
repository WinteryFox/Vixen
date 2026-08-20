#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>

#include "Resource.h"
#include "buffer/BufferFormat.h"
#include "image/ImageFormat.h"
#include "image/ImageView.h"

namespace Vixen {
    class Buffer;
    struct Image;

    struct ImageResourceDescription {
        ImageFormat format;

        ImageView view;
    };

    using ResourceDescription = std::variant<ImageResourceDescription, BufferFormat>;
    using ResourceState = std::variant<ImageState, BufferState>;
    using ImportedResource = std::variant<std::monostate, Image*, Buffer*>;

    struct ResourceNode {
        std::string name;

        ResourceType type;

        ResourceLifetime lifetime;

        ResourceDescription description;

        uint32_t latestVersion;

        ImportedResource importedResource{};

        std::optional<ResourceState> initialState;

        std::optional<ResourceState> finalState;
    };
}
