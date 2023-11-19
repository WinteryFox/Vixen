#pragma once

#include <volk.h>
#include <spdlog/spdlog.h>
#include <glm/glm.hpp>
#include <vulkan/vk_enum_string_helper.h>
#include "../Util.h"
#include "../ShaderModule.h"

#ifdef DEBUG

#include <spdlog/sinks/stdout_color_sinks.h>
#include <fmt/color.h>

#endif

namespace Vixen::Vk {
    template<typename T = std::runtime_error>
    static void checkVulkanResult(VkResult result, const std::string &message) {
        if (result != VK_SUCCESS)
            error<T>("{} ({})", message, string_VkResult(result));
    }

    static std::string getVersionString(glm::ivec3 version) {
        return fmt::format("{}.{}.{}", version.x, version.y, version.z);
    }

    static std::string getVersionString(const uint32_t version) {
        return fmt::format("{}.{}.{}", VK_API_VERSION_MAJOR(version), VK_API_VERSION_MINOR(version), VK_API_VERSION_PATCH(version));
    }

    static VkShaderStageFlagBits getVulkanShaderStage(const ShaderModule::Stage stage) {
        switch (stage) {
            case ShaderModule::Stage::VERTEX:
                return VK_SHADER_STAGE_VERTEX_BIT;
            case ShaderModule::Stage::FRAGMENT:
                return VK_SHADER_STAGE_FRAGMENT_BIT;
        }

        return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
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
                source = string_VkDebugUtilsMessageTypeFlagsEXT(messageType);
                break;
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
