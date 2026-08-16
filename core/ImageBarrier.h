#pragma once

#include "BarrierAccessFlags.h"
#include "image/ImageLayout.h"
#include "image/ImageSubresourceRange.h"

namespace Vixen {
    struct Image;

    struct ImageBarrier {
        Image* image;
        BarrierAccessFlags sourceAccess;
        BarrierAccessFlags destinationAccess;
        ImageLayout oldLayout;
        ImageLayout newLayout;
        ImageSubresourceRange subresources;
    };
}
