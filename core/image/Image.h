#pragma once
#include "ImageFormat.h"
#include "ImageView.h"

namespace Vixen {
    struct ImageFormat;
    struct ImageView;

    class Image {
    protected:
        ImageFormat format;

        ImageView view;

    public:
        Image(const ImageFormat &format, const ImageView &view);

        virtual ~Image() = default;

        [[nodiscard]] virtual ImageFormat getFormat() const;

        [[nodiscard]] virtual uint32_t getWidth() const;

        [[nodiscard]] virtual uint32_t getHeight() const;

        [[nodiscard]] virtual uint32_t getDepth() const;

        [[nodiscard]] virtual bool getMipmapCount() const;
    };
}
