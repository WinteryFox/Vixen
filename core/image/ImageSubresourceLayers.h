#pragma once

#include "ImageAspectFlags.h"

namespace Vixen {
    struct ImageSubresourceLayers {
        ImageAspectFlags aspect;
        uint32_t mipmap;
        uint32_t baseLayer;
        uint32_t layerCount;
    };
}
