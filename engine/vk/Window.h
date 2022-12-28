#pragma once

#include "../Window.h"

namespace Vixen::Engine::Vk {
    struct Window : Vixen::Engine::Window {
        Window(const std::string &title, const uint32_t &width, const uint32_t &height);

        ~Window();
    };
}
