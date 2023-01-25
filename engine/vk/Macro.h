#pragma once

#include <Volk/volk.h>
#include <spdlog/spdlog.h>
#include <glm/glm.hpp>
#include "../Util.h"

#ifdef DEBUG

#include <spdlog/sinks/stdout_color_sinks.h>
#include <fmt/color.h>

#endif

namespace Vixen::Engine {
    template<typename T = std::runtime_error>
    static inline void checkVulkanResult(VkResult result, const std::string &message) {
        if (result != VK_SUCCESS) {
#ifdef A
            error<T>("{} ({})", message, string_VkResult(result));
#else
            error<T>("{} ({})", message, result);
#endif
        }
    }

    /*template<typename T>
    static inline T getInstanceProcAddress(VkInstance instance, const std::string &function) {
        auto func = (T) vkGetInstanceProcAddr(instance, function.c_str());
        if (func == nullptr)
            spdlog::warn("Failed to load function {}", function);

        return func;
    }*/

    static inline std::string getVersionString(glm::vec3 version) {
        return fmt::format("{}.{}.{}", version.x, version.y, version.z);
    }

    static inline std::string getVersionString(uint32_t version) {
        return fmt::format("{}.{}.{}", VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version), VK_VERSION_PATCH(version));
    }

#ifdef DEBUG

    static VkBool32 VKAPI_CALL vkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                               VkDebugUtilsMessageTypeFlagsEXT messageType,
                                               const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                               [[maybe_unused]] void *pUserData) {
        spdlog::level::level_enum level;
        switch (messageSeverity) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                level = spdlog::level::trace;
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                level = spdlog::level::info;
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                level = spdlog::level::warn;
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                level = spdlog::level::err;
                break;
            default:
                level = spdlog::level::warn;
                spdlog::warn("Unknown level flag in vkDebugCallback");
                break;
        }

        std::string source;
        switch (messageType) {
            case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
                source = "Performance";
                break;
            case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
                source = "Validation";
                break;
            case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
                source = "General";
                break;
            default:
#ifdef A
                source = string_VkDebugUtilsMessageTypeFlagsEXT(messageType);
#else
                source = "UNKNOWN";
#endif
        }

        auto vkDebugLogger = spdlog::get("Vulkan");
        if (vkDebugLogger == nullptr)
            vkDebugLogger = spdlog::stdout_color_mt("Vulkan");
        vkDebugLogger->log(
                level,
                "[{}] {}",
                fmt::format(fmt::fg(fmt::terminal_color::magenta), source),
                pCallbackData->pMessage
        );
        return VK_FALSE;
    }

#endif
}
