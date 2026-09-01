#pragma once

namespace Vixen {
    struct Image;

    struct Framebuffer {
        Image* colorTarget = nullptr;
        Image* depthTarget = nullptr;

        virtual ~Framebuffer() = default;
    };
}
