#pragma once

#include <optional>
#include <map>
#include <vulkan/vulkan.hpp>

namespace Vixen::Engine {
    struct QueueFamily {
        uint32_t index;

        VkQueueFamilyProperties properties{};

        [[nodiscard]] bool hasFlags(VkQueueFlags flags) const {
            return properties.queueFlags & flags;
        }

        bool hasSurfaceSupport(VkPhysicalDevice device, VkSurfaceKHR surface) const {
            VkBool32 support = VK_FALSE;
            VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &support),
                     "Failed to query surface support")
            return support;
        }
    };

    struct GraphicsCard {
        VkPhysicalDevice device;

        VkPhysicalDeviceProperties properties;

        VkPhysicalDeviceFeatures features;

        std::vector<QueueFamily> queueFamilies;

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
    };
}
