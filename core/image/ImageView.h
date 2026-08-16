#pragma once

#include "ImageDataFormat.h"
#include "ImageSwizzle.h"

namespace Vixen {
    struct ImageView {
        ImageDataFormat format;
        ImageSwizzle swizzleRed;
        ImageSwizzle swizzleGreen;
        ImageSwizzle swizzleBlue;
        ImageSwizzle swizzleAlpha;
    };
}
