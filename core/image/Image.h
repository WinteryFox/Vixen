#pragma once

#include "ImageFormat.h"
#include "ImageView.h"

namespace Vixen {
    struct Image {
        ImageFormat format{};

        ImageView view{};

        virtual ~Image() = default;
    };
}
