#include "Device.h"

namespace Vixen::Engine {
    Device::Device(const std::vector<const char *> &extensions, GraphicsCard gpu, VkSurfaceKHR surface)
            : device(VK_NULL_HANDLE), gpu(gpu), surface(surface) {
        graphicsQueueFamily = gpu.getQueueFamilyWithFlags(VK_QUEUE_GRAPHICS_BIT)[0];
        presentQueueFamily = gpu.getSurfaceSupportedQueues(surface)[0];
        std::set<uint32_t> queueFamilies = {graphicsQueueFamily.index, presentQueueFamily.index};
        // TODO: Detect and select best graphics and present queues
        std::vector<VkDeviceQueueCreateInfo> queueInfos;
        for (const auto &family: queueFamilies) {
            VkDeviceQueueCreateInfo queueInfo{};
            queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex = family;
            queueInfo.queueCount = 1;
            float queuePriority = 1.0f;
            queueInfo.pQueuePriorities = &queuePriority;
            queueInfos.push_back(queueInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.queueCreateInfoCount = queueInfos.size();
        deviceInfo.pQueueCreateInfos = queueInfos.data();
        deviceInfo.pEnabledFeatures = &deviceFeatures;
        deviceInfo.enabledExtensionCount = extensions.size();
        deviceInfo.ppEnabledExtensionNames = extensions.data();

        spdlog::info("Creating new Vulkan device using \"{} ({})\"", gpu.properties.deviceName,
                     getVersionString(gpu.properties.driverVersion));
        checkVulkanResult(
                vkCreateDevice(gpu.device, &deviceInfo, nullptr, &device),
                "Failed to create Vulkan device"
        );
        volkLoadDevice(device);

        graphicsQueue = getQueueHandle(graphicsQueueFamily.index, 0);
        presentQueue = getQueueHandle(presentQueueFamily.index, 0);
    }

    Device::~Device() {
        vkDestroyDevice(device, nullptr);
    }

    VkQueue Device::getQueueHandle(uint32_t queueFamilyIndex, uint32_t queueIndex) const {
        VkQueue queue = VK_NULL_HANDLE;
        vkGetDeviceQueue(device, queueFamilyIndex, queueIndex, &queue);
        if (queue == VK_NULL_HANDLE)
            error("Failed to get queue handle for queue family {} and index {}", queueFamilyIndex, queueIndex);

        return queue;
    }

    const VkDevice Device::getDevice() const {
        return device;
    }

    const GraphicsCard &Device::getGpu() const {
        return gpu;
    }

    const VkSurfaceKHR Device::getSurface() const {
        return surface;
    }

    const VkQueue_T *Device::getGraphicsQueue() const {
        return graphicsQueue;
    }

    const VkQueue_T *Device::getPresentQueue() const {
        return presentQueue;
    }

    const QueueFamily &Device::getGraphicsQueueFamily() const {
        return graphicsQueueFamily;
    }

    const QueueFamily &Device::getPresentQueueFamily() const {
        return presentQueueFamily;
    }
}
