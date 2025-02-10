#pragma once

#include <glm/glm.hpp>

#include "ImageSubresourceLayers.h"

namespace Vixen {
    struct ImageCopyRegion {
        ImageSubresourceLayers sourceSubresources;
        glm::ivec3 sourceOffset;
        ImageSubresourceLayers destinationSubresources;
        glm::ivec3 destinationOffset;
        glm::uvec3 size;
    };
}
