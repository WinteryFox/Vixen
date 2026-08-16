#pragma once

namespace Vixen {
    struct Framebuffer {
        Image* colorTarget = nullptr;
        Image* depthTarget = nullptr;

        virtual ~Framebuffer() = default;
    };
}
