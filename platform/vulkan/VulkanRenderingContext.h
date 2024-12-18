#pragma once

#include <vector>
#include <volk.h>

#include "device/GraphicsCard.h"
#include "core/RenderingContext.h"

namespace Vixen {
    class VulkanRenderingContext final : public RenderingContext {
        uint32_t instanceApiVersion;

        std::vector<const char*> enabledInstanceExtensions;

        VkInstance instance;

        std::vector<GraphicsCard> driverDevices;

        std::vector<VkPhysicalDevice> physicalDevices;

        VkSurfaceKHR surface;

        void initializeVulkanVersion();

        void initializeInstanceExtensions();

        void initializeInstance();

        void initializeDevices();

    public:
        VulkanRenderingContext();

        ~VulkanRenderingContext() override;
    };
}
