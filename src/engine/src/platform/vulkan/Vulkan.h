#pragma once

#include <Volk/volk.h>
#include <glm/glm.hpp>
#include <core/shader/ShaderModule.h>
#include <spdlog/spdlog.h>
#include <vulkan/vk_enum_string_helper.h>

#include "core/LoadAction.h"
#include "core/StoreAction.h"
#include "exception/VulkanException.h"
#include "shader/VulkanShaderModule.h"

#ifdef DEBUG

#include <fmt/color.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#endif

namespace Vixen {
    static bool isDepthFormat(const VkFormat format) {
        switch (format) {
            case VK_FORMAT_D32_SFLOAT:
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
            case VK_FORMAT_D24_UNORM_S8_UINT:
                return true;
            default:
                return false;
        }
    }

    static VkAttachmentLoadOp toVkLoadAction(const LoadAction loadAction) {
        switch (loadAction) {
            case LoadAction::Load:
                return VK_ATTACHMENT_LOAD_OP_LOAD;
                break;
            case LoadAction::Clear:
                return VK_ATTACHMENT_LOAD_OP_CLEAR;
                break;
            case LoadAction::DontCare:
                return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                break;
        }

        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }

    static VkAttachmentStoreOp toVkStoreAction(const StoreAction storeAction) {
        switch (storeAction) {
            case StoreAction::Store:
                return VK_ATTACHMENT_STORE_OP_STORE;
                break;
            case StoreAction::Resolve:
                return VK_ATTACHMENT_STORE_OP_NONE;
                break;
            case StoreAction::StoreAndResolve:
                return VK_ATTACHMENT_STORE_OP_STORE;
                break;
            case StoreAction::DontCare:
                return VK_ATTACHMENT_STORE_OP_DONT_CARE;
                break;
        }

        return VK_ATTACHMENT_STORE_OP_NONE;
    }

    static void checkVulkanResult(const VkResult result, const std::string &message) {
        if (result != VK_SUCCESS)
            throw VulkanException(message);
    }

    static std::string getVersionString(glm::ivec3 version) {
        return fmt::format("{}.{}.{}", version.x, version.y, version.z);
    }

    static std::string getVersionString(const uint32_t version) {
        return fmt::format("{}.{}.{}", VK_API_VERSION_MAJOR(version), VK_API_VERSION_MINOR(version),
                           VK_API_VERSION_PATCH(version));
    }

    static VkShaderStageFlagBits getVulkanShaderStage(const ShaderModule::Stage stage) {
        switch (stage) {
            case ShaderModule::Stage::Vertex:
                return VK_SHADER_STAGE_VERTEX_BIT;
            case ShaderModule::Stage::Fragment:
                return VK_SHADER_STAGE_FRAGMENT_BIT;
            default:
                throw std::runtime_error("Unknown shader stage");
        }

        return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
    }

#ifdef DEBUG
    static VkBool32 VKAPI_CALL vkDebugCallback(
        const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        const VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        [[maybe_unused]] void *pUserData
    ) {
        spdlog::level::level_enum level;
        switch (messageSeverity) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                level = spdlog::level::debug;
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
