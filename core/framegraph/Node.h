#pragma once

#include <string>
#include <variant>

#include "Resource.h"
#include "buffer/BufferFormat.h"
#include "image/ImageFormat.h"

namespace Vixen {
    using ResourceDescription = std::variant<ImageFormat, BufferFormat>;

    struct ResourceNode {
        std::string name;

        ResourceType type;

        ResourceLifetime lifetime;

        ResourceDescription description;
    };
}
