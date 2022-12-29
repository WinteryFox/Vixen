#pragma once

namespace Vixen::Engine {
    struct Monitor {
        std::string name;
        int width;
        int height;
        int refreshRate;
        int blueBits;
        int redBits;
        int greenBits;
        bool isPrimary;
    };
}
