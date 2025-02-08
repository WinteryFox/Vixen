#pragma once

#include <vector>
#include <volk.h>
#include <glm/glm.hpp>

#include "GraphicsCard.h"
#include "VulkanSurface.h"
#include "core/RenderingContext.h"

namespace Vixen {
    class VulkanRenderingContext final : public RenderingContext {
        uint32_t instanceApiVersion;

        std::vector<const char *> enabledInstanceExtensions;

        VkInstance instance;

        std::vector<GraphicsCard> physicalDevices;

        void initializeVulkanVersion();

        void initializeInstanceExtensions();

        void initializeInstance(const std::string &applicationName, const glm::ivec3 &applicationVersion);

        void initializeDevices();

    public:
        explicit VulkanRenderingContext(const std::string &applicationName, const glm::ivec3 &applicationVersion);

        ~VulkanRenderingContext() override;

        RenderingDevice *createDevice() override;

        bool supportsPresent(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, const VulkanSurface *surface);

        Surface *createSurface(Window *window) override;

        void destroySurface(Surface *surface) override;

        GraphicsCard getPhysicalDevice(uint32_t index);

        [[nodiscard]] VkInstance getInstance() const;

        [[nodiscard]] uint32_t getInstanceApiVersion() const;
    };
}
