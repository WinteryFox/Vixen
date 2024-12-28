#include "Image.h"

namespace Vixen {
    Image::Image(const ImageFormat &format, const ImageView &view)
        : format(format),
          view(view) {

    }

    ImageFormat Image::getFormat() const {
        return format;
    }

    uint32_t Image::getWidth() const {
        return format.width;
    }

    uint32_t Image::getHeight() const {
        return format.height;
    }

    uint32_t Image::getDepth() const {
        return format.depth;
    }

    bool Image::getMipmapCount() const {
        return format.mipmapCount;
    }
}
