#pragma once

#include <vector>
#include <volk.h>
#include <glm/glm.hpp>

#include "VulkanSurface.h"
#include "core/RenderingContextDriver.h"

namespace Vixen {
    class VulkanRenderingContextDriver final : public RenderingContextDriver {
        uint32_t instanceApiVersion;

        std::vector<const char *> enabledInstanceExtensions;

        VkInstance instance;

        std::vector<DriverDevice> driverDevices;
        std::vector<VkPhysicalDevice> physicalDevices;
        std::vector<std::vector<VkQueueFamilyProperties> > deviceQueueFamilyProperties;

        void initializeVulkanVersion();

        void initializeInstanceExtensions();

        void initializeInstance(
            const std::string &applicationName,
            const glm::ivec3 &applicationVersion
        );

        void initializeDevices();

    public:
        explicit VulkanRenderingContextDriver(
            const std::string &applicationName,
            const glm::ivec3 &applicationVersion
        );

        ~VulkanRenderingContextDriver() override;

        std::vector<DriverDevice> getDevices() override;

        bool deviceSupportsPresent(
            uint32_t deviceIndex,
            Surface *surface
        ) override;

        [[nodiscard]] uint32_t getQueueFamilyCount(
            uint32_t deviceIndex
        ) const;

        [[nodiscard]] VkQueueFamilyProperties getQueueFamilyProperties(
            uint32_t deviceIndex,
            uint32_t queueFamilyIndex
        ) const;

        static bool queueFamilySupportsPresent(
            VkPhysicalDevice physicalDevice,
            uint32_t queueFamilyIndex,
            const VulkanSurface *surface
        );

        [[nodiscard]] VkPhysicalDevice getPhysicalDevice(
            uint32_t deviceIndex
        ) const;

        RenderingDeviceDriver *createRenderingDeviceDriver(
            uint32_t deviceIndex,
            uint32_t frameCount
        ) override;

        Surface *createSurface(
            Window *window
        ) override;

        void destroySurface(
            Surface *surface
        ) override;

        [[nodiscard]] VkInstance getInstance() const;

        [[nodiscard]] uint32_t getInstanceApiVersion() const;
    };
}
