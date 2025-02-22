#pragma once
#include "ImageSwizzle.h"

namespace Vixen {
    enum class ImageSwizzle;

    struct ImageView {
        DataFormat format;
        ImageSwizzle swizzleRed;
        ImageSwizzle swizzleGreen;
        ImageSwizzle swizzleBlue;
        ImageSwizzle swizzleAlpha;
    };
}
