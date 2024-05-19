#pragma once

#include <map>
#include <algorithm>

namespace Vixen {
    struct QueueFamily {
        uint32_t index = -1;

        VkQueueFamilyProperties properties{};

        [[nodiscard]] bool hasFlags(VkQueueFlags flags) const {
            return properties.queueFlags & flags;
        }

        bool hasSurfaceSupport(VkPhysicalDevice device, VkSurfaceKHR surface) const {
            VkBool32 support = VK_FALSE;
            checkVulkanResult(
                vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &support),
                "Failed to query surface support"
            );
            return support;
        }
    };

    class GraphicsCard {
    public:
        VkPhysicalDevice device;

        VkPhysicalDeviceProperties properties;

        VkPhysicalDeviceFeatures features;

        std::vector<QueueFamily> queueFamilies;

        std::vector<VkExtensionProperties> extensions;

        std::vector<VkLayerProperties> layers;

        explicit GraphicsCard(VkPhysicalDevice device) : device(device), properties(), features() {
            vkGetPhysicalDeviceProperties(device, &properties);
            vkGetPhysicalDeviceFeatures(device, &features);

            uint32_t queueFamilyCount;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties> qf(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, qf.data());

            queueFamilies.resize(queueFamilyCount);
            for (uint32_t x = 0; x < queueFamilyCount; x++) {
                queueFamilies[x] = QueueFamily{
                    .index = x,
                    .properties = qf[x]
                };
            }

            extensions = getSupportedExtensions(device);
            layers = getSupportedLayers(device);
        }

        [[nodiscard]] std::vector<QueueFamily> getQueueFamilyWithFlags(const VkQueueFlags flags) const {
            std::vector<QueueFamily> families{};
            for (auto queue: queueFamilies)
                if (queue.hasFlags(flags))
                    families.push_back(queue);

            return families;
        }

        std::vector<QueueFamily> getSurfaceSupportedQueues(VkSurfaceKHR surface) const {
            std::vector<QueueFamily> families{};
            for (auto queue: queueFamilies)
                if (queue.hasSurfaceSupport(device, surface))
                    families.push_back(queue);

            return families;
        }

        VkSurfaceCapabilitiesKHR getSurfaceCapabilities(VkSurfaceKHR surface) const {
            VkSurfaceCapabilitiesKHR capabilities;
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);

            return capabilities;
        }

        std::vector<VkSurfaceFormatKHR> getSurfaceFormats(VkSurfaceKHR surface) const {
            uint32_t count;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr);
            std::vector<VkSurfaceFormatKHR> formats{count};
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, formats.data());

            return formats;
        }

        std::vector<VkPresentModeKHR> getPresentModes(VkSurfaceKHR surface) const {
            uint32_t count;
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr);
            std::vector<VkPresentModeKHR> modes{count};
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, modes.data());

            return modes;
        }

        static std::vector<VkExtensionProperties> getSupportedExtensions(VkPhysicalDevice device) {
            uint32_t count;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);

            std::vector<VkExtensionProperties> extensions(count);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());
            return extensions;
        }

        [[nodiscard]] bool isExtensionSupported(const std::string &extension) const {
            return std::ranges::find_if(
                       extensions,
                       [extension](const VkExtensionProperties &props) {
                           return extension == props.extensionName;
                       }
                   ) != std::end(extensions);
        }

        [[nodiscard]] bool supportsExtensions(const std::vector<const char *> &requestedExtensions) const {
            if (!std::ranges::all_of(requestedExtensions,
                                     [this](auto extension) { return isExtensionSupported(extension); }))
                return false;

            return true;
        }

        static std::vector<VkLayerProperties> getSupportedLayers(VkPhysicalDevice device) {
            uint32_t count;
            vkEnumerateDeviceLayerProperties(device, &count, nullptr);

            std::vector<VkLayerProperties> layers(count);
            vkEnumerateDeviceLayerProperties(device, &count, layers.data());
            return layers;
        }

        [[nodiscard]] bool isLayerSupported(const std::string &layer) const {
            return std::ranges::find_if(
                       layers,
                       [layer](const VkLayerProperties &props) {
                           return layer == props.layerName;
                       }
                   ) != std::end(layers);
        }

        [[nodiscard]] VkFormat
        pickFormat(const std::vector<VkFormat> &formats, VkImageTiling tiling, VkFormatFeatureFlags feats) const {
            for (const auto &format: formats) {
                VkFormatProperties props;
                vkGetPhysicalDeviceFormatProperties(device, format, &props);

                if ((tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & feats) == feats) ||
                    (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & feats) == feats))
                    return format;
            }

            spdlog::error("Failed to pick suitable format");
            throw std::runtime_error("Failed to pick suitable format");
        }

        [[nodiscard]] VkSampleCountFlagBits getMaxSampleCount() const {
            const auto counts = properties.limits.framebufferColorSampleCounts &
                                properties.limits.framebufferDepthSampleCounts;

            if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
            if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
            if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
            if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
            if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
            if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

            return VK_SAMPLE_COUNT_1_BIT;
        }

        [[nodiscard]] VkFormatProperties getFormatProperties(const VkFormat format) const {
            VkFormatProperties properties;
            vkGetPhysicalDeviceFormatProperties(device, format, &properties);

            return properties;
        }
    };
}
