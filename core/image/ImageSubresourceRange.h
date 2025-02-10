#pragma once
#include "ImageAspectFlags.h"

namespace Vixen {
    struct ImageSubresourceRange {
        ImageAspectFlags aspect;
        uint32_t baseMipmap;
        uint32_t mipmapCount;
        uint32_t baseLayer;
        uint32_t layerCount;
    };
}
