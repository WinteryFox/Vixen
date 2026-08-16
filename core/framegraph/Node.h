#pragma once

#include <cstdint>
#include <string>
#include <variant>

#include "Resource.h"
#include "buffer/BufferFormat.h"
#include "image/ImageFormat.h"
#include "image/ImageView.h"

namespace Vixen {
    struct ImageResourceDescription {
        ImageFormat format;

        ImageView view;
    };

    using ResourceDescription = std::variant<ImageResourceDescription, BufferFormat>;

    struct ResourceNode {
        std::string name;

        ResourceType type;

        ResourceLifetime lifetime;

        ResourceDescription description;

        uint32_t latestVersion;

        bool imported = false;
    };
}
