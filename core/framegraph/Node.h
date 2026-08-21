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

        /**
         * Created resources begin at an uninitialized version zero. Imported
         * resources begin at an externally initialized version zero. Producer
         * and uninitialized-read validation is performed when the complete graph
         * is compiled, where all passes and resources usages are available.
         */
        uint32_t latestVersion;

        ImportedResource importedResource{};

        std::optional<ResourceState> initialState;

        std::optional<ResourceState> finalState;
    };
}
