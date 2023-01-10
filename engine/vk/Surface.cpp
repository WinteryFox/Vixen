#include "Surface.h"

#include <utility>

namespace Vixen::Engine::Vk {
    Surface::Surface(std::shared_ptr<Instance> instance, VkSurfaceKHR surface) : instance(std::move(instance)), surface(surface) {}

    Surface::Surface(const std::shared_ptr<Instance>& instance, const Window &window) : instance(instance), surface(VK_NULL_HANDLE) {
        glfwCreateWindowSurface(instance->instance, window.window, nullptr, &surface);
    }

    Surface::~Surface() {
        vkDestroySurfaceKHR(instance->instance, surface, nullptr);
    }
}
