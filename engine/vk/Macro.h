#pragma once

#include <spdlog/spdlog.h>

#define VK_CHECK(f, m) { \
    VkResult _rr = (f); \
    if (_rr != VK_SUCCESS) { \
        spdlog::error("Vk call failed; {}", m); \
        throw std::runtime_error("Vk call failed"); \
    }\
}

template<typename T>
static T getInstanceProcAddress(VkInstance instance, const std::string &function) {
    auto func = (T) vkGetInstanceProcAddr(instance, function.c_str());
    if (func == nullptr)
        spdlog::warn("Failed to load function {}", function);

    return func;
}
