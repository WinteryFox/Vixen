#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <volk.h>
#include <glm/vec3.hpp>

#include "core/rendering/RenderingContextDriver.h"

namespace Vixen {
    struct VulkanSurface;

    class VulkanRenderingContextDriver final : public RenderingContextDriver {
        uint32_t instanceApiVersion;

        std::vector<std::string> enabledInstanceExtensions;

        VkInstance instance;

        struct PhysicalDeviceRecord {
            VkPhysicalDevice handle = VK_NULL_HANDLE;
            VkPhysicalDeviceProperties properties{};
            VkPhysicalDeviceMemoryProperties memoryProperties{};
            DriverDevice description{};
            std::vector<VkQueueFamilyProperties> queueFamilies;
        };

        std::vector<PhysicalDeviceRecord> physicalDevices;

        void initializeVulkanVersion();

        void initializeInstanceExtensions();

        void initializeInstance(
            const std::string& applicationName,
            const glm::ivec3& applicationVersion
        );

        void initializeDevices();

    public:
        explicit VulkanRenderingContextDriver(
            const std::string& applicationName,
            const glm::ivec3& applicationVersion
        );

        ~VulkanRenderingContextDriver() override;

        std::vector<DriverDevice> getDevices() override;

        bool deviceSupportsPresent(
            uint32_t deviceIndex,
            Surface* surface
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
            const VulkanSurface* surface
        );

        [[nodiscard]] VkPhysicalDevice getPhysicalDevice(
            uint32_t deviceIndex
        ) const;

        RenderingDeviceDriver* createRenderingDeviceDriver(
            uint32_t deviceIndex,
            uint32_t frameCount
        ) override;

        void destroyRenderingDeviceDriver(RenderingDeviceDriver* renderingDeviceDriver) override;

        auto createSurface(Window* window) -> std::expected<Surface*, Error> override;

        bool getSurfaceNeedsResize(Surface* surface) override;

        void setSurfaceNeedsResize(Surface* surface, bool needsResize) override;

        void setSurfaceSize(Surface* surface, uint32_t width, uint32_t height) override;

        void setSurfaceVSyncMode(Surface* surface, VSyncMode vsyncMode) override;

        void setSurfaceWindowMode(Surface* surface, WindowMode mode) override;

        void destroySurface(
            Surface* surface
        ) override;

        [[nodiscard]] VkInstance getInstance() const;

        [[nodiscard]] uint32_t getInstanceApiVersion() const;
    };
}
