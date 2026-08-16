#pragma once

#include <cstdint>
#include <glm/vec3.hpp>

#include "core/image/ImageSubresourceLayers.h"

namespace Vixen {
    struct BufferImageCopyRegion {
        uint64_t bufferOffset;
        ImageSubresourceLayers imageSubresourceLayers;
        glm::ivec3 imageOffset;
        glm::uvec3 imageRegionSize;
    };
}
