#pragma once

#include <cstdint>

#include "ImageSamples.h"
#include "ImageType.h"
#include "ImageUsage.h"
#include "core/ImageDataFormat.h"

namespace Vixen {
    struct ImageFormat {
        ImageDataFormat format;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t layerCount;
        uint32_t mipmapCount;
        ImageType type;
        ImageSamples samples;
        ImageUsageFlags usage;
    };
}
