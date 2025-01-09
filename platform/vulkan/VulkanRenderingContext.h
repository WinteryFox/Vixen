#pragma once

#include <vector>
#include <volk.h>

#include "GraphicsCard.h"
#include "VulkanSurface.h"
#include "core/RenderingContext.h"

namespace Vixen {
    class VulkanRenderingContext final : public RenderingContext {
        uint32_t instanceApiVersion;

        std::vector<const char*> enabledInstanceExtensions;

        VkInstance instance;

        std::vector<GraphicsCard> physicalDevices;

        VkSurfaceKHR surface;

        void initializeVulkanVersion();

        void initializeInstanceExtensions();

        void initializeInstance(const std::string &applicationName);

        void initializeDevices();

    public:
        explicit VulkanRenderingContext(const std::string &applicationName);

        ~VulkanRenderingContext() override;

        GraphicsCard getPhysicalDevice(uint32_t index);

        [[nodiscard]] VkInstance getInstance() const;

        bool supportsPresent(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VulkanSurface *surface);
    };
}
