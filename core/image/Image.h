#pragma once
#include "ImageFormat.h"
#include "ImageView.h"

namespace Vixen {
    struct ImageFormat;
    struct ImageView;

    struct Image {
        ImageFormat format{};

        ImageView view{};
    };
}
