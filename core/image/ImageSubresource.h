#pragma once
#include <cstdint>

#include "ImageAspect.h"

namespace Vixen {
    struct ImageSubresource {
        ImageAspect aspect;
        uint32_t layer;
        uint32_t mipmap;
    };
}
