#pragma once

#include <optional>
#include <map>
#include <algorithm>

namespace Vixen::Engine {
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

        std::vector<QueueFamily> getQueueFamilyWithFlags(VkQueueFlags flags) {
            std::vector<QueueFamily> families{};
            for (auto queue: queueFamilies)
                if (queue.hasFlags(flags))
                    families.push_back(queue);

            return families;
        }

        std::vector<QueueFamily> getSurfaceSupportedQueues(VkSurfaceKHR surface) {
            std::vector<QueueFamily> families{};
            for (auto queue: queueFamilies)
                if (queue.hasSurfaceSupport(device, surface))
                    families.push_back(queue);

            return families;
        }

        static std::vector<VkExtensionProperties> getSupportedExtensions(VkPhysicalDevice device) {
            uint32_t count;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);

            std::vector<VkExtensionProperties> extensions(count);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());
            return extensions;
        }

        [[nodiscard]] bool isExtensionSupported(const std::string &extension) const {
            return std::find_if(
                    extensions.begin(),
                    extensions.end(),
                    [extension](VkExtensionProperties props) { return extension == props.extensionName; }
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
            return std::find_if(
                    layers.begin(),
                    layers.end(),
                    [layer](VkLayerProperties props) { return layer == props.layerName; }
            ) != std::end(layers);
        }
    };
}
