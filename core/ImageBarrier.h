#pragma once

#include "image/Image.h"
#include "image/ImageLayout.h"
#include "image/ImageSubresourceRange.h"

namespace Vixen {
    struct ImageBarrier {
        Image *image;
        BarrierAccessFlags sourceAccess;
        BarrierAccessFlags destinationAccess;
        ImageLayout oldLayout;
        ImageLayout newLayout;
        ImageSubresourceRange subresources;
    };
}
