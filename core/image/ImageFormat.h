#pragma once

#include <cstdint>

#include "ImageSamples.h"
#include "ImageType.h"
#include "ImageUsage.h"
#include "core/DataFormat.h"

namespace Vixen {
    struct ImageFormat {
        DataFormat format;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t layerCount;
        uint32_t mipmapCount;
        ImageType type;
        ImageSamples samples;
        ImageUsage usage;
    };
}
