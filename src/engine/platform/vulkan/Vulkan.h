#pragma once

#include <Volk/volk.h>
#include <vulkan/vk_enum_string_helper.h>

#ifdef DEBUG

#include <fmt/color.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#endif

#include <core/shader/ShaderModule.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

#include "core/LoadAction.h"
#include "core/Samples.h"
#include "core/StoreAction.h"
#include "exception/VulkanException.h"

namespace Vixen {
    [[maybe_unused]] static bool isDepthFormat(const VkFormat format) {
        switch (format) {
            case VK_FORMAT_D32_SFLOAT:
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
            case VK_FORMAT_D24_UNORM_S8_UINT:
                return true;

            default:
                return false;
        }
    }

    [[maybe_unused]] static VkSampleCountFlagBits toVkSampleCountFlagBits(const Samples &samples) {
        switch (samples) {
                using enum Samples;

            case None:
                return VK_SAMPLE_COUNT_1_BIT;
                break;

            case MSAA2x:
                return VK_SAMPLE_COUNT_2_BIT;
                break;

            case MSAA4x:
                return VK_SAMPLE_COUNT_4_BIT;
                break;

            case MSAA8x:
                return VK_SAMPLE_COUNT_8_BIT;
                break;
        }

        throw std::runtime_error("Unsupported sample count");
    }

    [[maybe_unused]] static VkAttachmentLoadOp toVkLoadAction(const LoadAction loadAction) {
        switch (loadAction) {
                using enum LoadAction;

            case Load:
                return VK_ATTACHMENT_LOAD_OP_LOAD;
                break;

            case Clear:
                return VK_ATTACHMENT_LOAD_OP_CLEAR;
                break;

            case DontCare:
                return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                break;
        }

        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }

    [[maybe_unused]] static VkAttachmentStoreOp toVkStoreAction(const StoreAction storeAction) {
        switch (storeAction) {
                using enum StoreAction;

            case Store:
                return VK_ATTACHMENT_STORE_OP_STORE;
                break;

            case Resolve:
                return VK_ATTACHMENT_STORE_OP_NONE;
                break;

            case StoreAndResolve:
                return VK_ATTACHMENT_STORE_OP_STORE;
                break;

            case DontCare:
                return VK_ATTACHMENT_STORE_OP_DONT_CARE;
                break;
        }

        throw std::runtime_error("Unsupported store action");
    }

    [[maybe_unused]] static void checkVulkanResult(const VkResult result, const std::string &message = "") {
        if (result != VK_SUCCESS)
            throw VulkanException(message);
    }

    [[maybe_unused]] static std::string getVersionString(glm::ivec3 version) {
        return fmt::format("{}.{}.{}", version.x, version.y, version.z);
    }

    [[maybe_unused]] static std::string getVersionString(const uint32_t version) {
        return fmt::format("{}.{}.{}", VK_API_VERSION_MAJOR(version), VK_API_VERSION_MINOR(version),
                           VK_API_VERSION_PATCH(version));
    }

    [[maybe_unused]] static VkShaderStageFlagBits getVulkanShaderStage(const ShaderResources::Stage stage) {
        switch (stage) {
                using enum ShaderResources::Stage;

            case All:
                return VK_SHADER_STAGE_ALL;

            case Vertex:
                return VK_SHADER_STAGE_VERTEX_BIT;

            case Fragment:
                return VK_SHADER_STAGE_FRAGMENT_BIT;

            default:
                return VK_SHADER_STAGE_ALL;
        }

        throw std::runtime_error("Unknown shader stage");
    }

    [[maybe_unused]] static VkVertexInputRate toVkVertexInputRate(const ShaderResources::Rate rate) {
        switch (rate) {
            case ShaderResources::Rate::Vertex:
                return VK_VERTEX_INPUT_RATE_VERTEX;

            case ShaderResources::Rate::Instance:
                return VK_VERTEX_INPUT_RATE_INSTANCE;
        }

        throw std::runtime_error("Unknown input rate");
    }

    [[maybe_unused]] static VkShaderStageFlags toVkShaderStage(const ShaderResources::Stage stage) {
        switch (stage) {
            case ShaderResources::Stage::All:
                return VK_SHADER_STAGE_ALL;

            case ShaderResources::Stage::Vertex:
                return VK_SHADER_STAGE_VERTEX_BIT;

            case ShaderResources::Stage::Fragment:
                return VK_SHADER_STAGE_FRAGMENT_BIT;
        }

        throw std::runtime_error("Unknown shader stage");
    }

#ifdef DEBUG
    [[maybe_unused]] static VkBool32 vkDebugCallback(
        const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        const VkDebugUtilsMessageTypeFlagsEXT messageTypes,
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
        switch (messageTypes) {
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
                source = string_VkDebugUtilsMessageTypeFlagsEXT(messageTypes);
                break;
        }

        auto vkDebugLogger = spdlog::get("Vulkan");
        if (vkDebugLogger == nullptr)
            vkDebugLogger = spdlog::stdout_color_mt("Vulkan");

        vkDebugLogger->log(
            level,
            "[{}] {}",
            format(fg(fmt::terminal_color::magenta), fmt::runtime(source)),
            pCallbackData->pMessage
        );

        return VK_FALSE;
    };
#endif
}
