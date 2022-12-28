#pragma once

#include <cstdint>
#include <string>
#include "Window.h"

namespace Vixen::Editor {
    class GlWindow : Window {
    public:
        GlWindow(const std::string &title, const uint32_t &width, const uint32_t &height);
    };
}
