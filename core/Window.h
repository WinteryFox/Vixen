#pragma once

#include <GLFW/glfw3.h>

#include "Surface.h"

namespace Vixen {
    struct Window {
        GLFWwindow *window;
        Surface *surface;
        Swapchain *swapchain;
    };
}
