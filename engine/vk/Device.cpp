#include "Device.h"

namespace Vixen::Vk {
    Device::Device(
            const Instance &instance,
            const std::vector<const char *> &extensions,
            GraphicsCard gpu,
            VkSurfaceKHR surface
    ) : device(VK_NULL_HANDLE),
        allocator(VK_NULL_HANDLE),
        gpu(gpu),
        surface(surface) {
        graphicsQueueFamily = gpu.getQueueFamilyWithFlags(VK_QUEUE_GRAPHICS_BIT)[0];
        presentQueueFamily = gpu.getSurfaceSupportedQueues(surface)[0];
        transferQueueFamily = gpu.getQueueFamilyWithFlags(VK_QUEUE_TRANSFER_BIT)[0];

        std::set<uint32_t> queueFamilies = {
                graphicsQueueFamily.index,
                presentQueueFamily.index,
                transferQueueFamily.index,
        };

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

        spdlog::info(
                "Creating new Vulkan device using GPU \"{}\" Vulkan {}",
                gpu.properties.deviceName,
                getVersionString(gpu.properties.apiVersion)
        );
        checkVulkanResult(
                vkCreateDevice(gpu.device, &deviceInfo, nullptr, &device),
                "Failed to create Vulkan device"
        );
        volkLoadDevice(device);

        VmaVulkanFunctions vulkanFunctions{
                .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
                .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
                .vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties,
                .vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties,
                .vkAllocateMemory = vkAllocateMemory,
                .vkFreeMemory = vkFreeMemory,
                .vkMapMemory = vkMapMemory,
                .vkUnmapMemory = vkUnmapMemory,
                .vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges,
                .vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges,
                .vkBindBufferMemory = vkBindBufferMemory,
                .vkBindImageMemory = vkBindImageMemory,
                .vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements,
                .vkGetImageMemoryRequirements = vkGetImageMemoryRequirements,
                .vkCreateBuffer = vkCreateBuffer,
                .vkDestroyBuffer = vkDestroyBuffer,
                .vkCreateImage = vkCreateImage,
                .vkDestroyImage = vkDestroyImage,
                .vkCmdCopyBuffer = vkCmdCopyBuffer,
                .vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2,
                .vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2,
                .vkBindBufferMemory2KHR = vkBindBufferMemory2,
                .vkBindImageMemory2KHR = vkBindImageMemory2,
                .vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2,
                .vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements,
                .vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements,
        };

        VmaAllocatorCreateInfo allocatorInfo{
                .physicalDevice = gpu.device,
                .device = device,
                .pVulkanFunctions = &vulkanFunctions,
                .instance = instance.instance,
                .vulkanApiVersion = VK_API_VERSION_1_3,
        };
        vmaCreateAllocator(&allocatorInfo, &allocator);

        graphicsQueue = getQueueHandle(graphicsQueueFamily.index, 0);

        presentQueue = getQueueHandle(presentQueueFamily.index, 0);

        transferQueue = getQueueHandle(transferQueueFamily.index, 0);
        transferCommandPool = std::make_unique<VkCommandPool>(
                device,
                transferQueueFamily.index,
                VkCommandPool::Usage::TRANSIENT,
                true
        );
    }

    Device::~Device() {
        vkDeviceWaitIdle(device);
        transferCommandPool = nullptr;
        vmaDestroyAllocator(allocator);
        vkDestroyDevice(device, nullptr);
    }

    VkQueue Device::getQueueHandle(uint32_t queueFamilyIndex, uint32_t queueIndex) const {
        VkQueue queue = VK_NULL_HANDLE;
        vkGetDeviceQueue(device, queueFamilyIndex, queueIndex, &queue);
        if (queue == VK_NULL_HANDLE)
            error("Failed to get queue handle for queue family {} and index {}", queueFamilyIndex, queueIndex);

        return queue;
    }

    VkDevice Device::getDevice() const {
        return device;
    }

    const GraphicsCard &Device::getGpu() const {
        return gpu;
    }

    VkSurfaceKHR Device::getSurface() const {
        return surface;
    }


    const QueueFamily &Device::getGraphicsQueueFamily() const {
        return graphicsQueueFamily;
    }

    VkQueue Device::getGraphicsQueue() const {
        return graphicsQueue;
    }

    const QueueFamily &Device::getTransferQueueFamily() const {
        return transferQueueFamily;
    }

    VkQueue Device::getTransferQueue() const {
        return transferQueue;
    }

    std::unique_ptr<VkCommandPool> &Device::getTransferCommandPool() {
        return transferCommandPool;
    }

    const QueueFamily &Device::getPresentQueueFamily() const {
        return presentQueueFamily;
    }

    VkQueue Device::getPresentQueue() const {
        return presentQueue;
    }

    VmaAllocator Device::getAllocator() const {
        return allocator;
    }

    void Device::waitIdle() const {
        vkDeviceWaitIdle(device);
    }
}
