#pragma once

#include "Window.h"
#include "Instance.h"

namespace Vixen::Engine::Vk {
    class Surface {
        VkSurfaceKHR surface;

        std::shared_ptr<Instance> instance;

    public:
        Surface(std::shared_ptr<Instance> instance, VkSurfaceKHR surface);

        Surface(const std::shared_ptr<Instance>& instance, const Vk::Window &window);

        ~Surface();
    };
}
