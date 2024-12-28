#pragma once

#include "buffer/Buffer.h"
#include "image/Image.h"
#include "image/ImageFormat.h"
#include "image/ImageView.h"

namespace Vixen {
    class RenderingDevice {
    public:
        virtual ~RenderingDevice() = default;

        virtual Buffer createBuffer(Buffer::Usage usage, uint32_t count, uint32_t stride) = 0;

        virtual Image createImage(const ImageFormat &format, const ImageView &view) = 0;
    };
}
