#pragma once
#include "Image.h"

namespace Vixen {
    class Image2D : public Image {
    public:
        Image2D(const ImageFormat &format, const ImageView &view);
    };
}
