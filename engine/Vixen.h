#pragma once

#include "Renderer.h"
#include "Window.h"

namespace Vixen::Engine {
    class Vixen {
    protected:
        std::unique_ptr<Renderer> renderer;
    };
}
