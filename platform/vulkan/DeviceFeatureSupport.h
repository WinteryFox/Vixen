#pragma once

#include <volk.h>

struct DeviceFeatureSupport {
    VkPhysicalDeviceFeatures2 core{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2
    };

    VkPhysicalDeviceVulkan12Features vulkan12{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES
    };

    VkPhysicalDeviceVulkan13Features vulkan13{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES
    };
};
