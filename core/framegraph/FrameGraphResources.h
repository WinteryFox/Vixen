#pragma once

#include <span>
#include <variant>

#include "Resource.h"

namespace Vixen {
    class Buffer;
    struct Image;
    using ResourceObject = std::variant<std::monostate, Image*, Buffer*>;

    class FrameGraphResources {
        std::span<const ResourceObject> resources;

    public:
        explicit FrameGraphResources(std::span<const ResourceObject> resources) : resources(resources) {}

        [[nodiscard]] Image* get(const ImageHandle handle) const {
            return std::get<Image*>(resources[handle.id.index]);
        }

        [[nodiscard]] Buffer* get(const BufferHandle handle) const {
            return std::get<Buffer*>(resources[handle.id.index]);
        }
    };
}
