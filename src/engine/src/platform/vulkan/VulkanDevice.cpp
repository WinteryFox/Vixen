#define VMA_IMPLEMENTATION
#include "VulkanDevice.h"

#include <set>

#include "commandbuffer/VulkanCommandPool.h"
#include "core/CommandPoolUsage.h"

namespace Vixen {
    VulkanDevice::VulkanDevice(
        const std::shared_ptr<Instance> &instance,
        const std::vector<const char *> &extensions,
        GraphicsCard gpu,
        VkSurfaceKHR surface
    ) : instance(instance),
        gpu(gpu),
        device(VK_NULL_HANDLE),
        allocator(VK_NULL_HANDLE),
        surface(surface),
        transferCommandPool(nullptr) {
        graphicsQueueFamily = gpu.getQueueFamilyWithFlags(VK_QUEUE_GRAPHICS_BIT)[0];
        presentQueueFamily = gpu.getSurfaceSupportedQueues(surface)[0];
        transferQueueFamily = gpu.getQueueFamilyWithFlags(VK_QUEUE_TRANSFER_BIT)[0];

        std::set queueFamilies = {
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

        VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
            .pNext = nullptr,
            .dynamicRendering = VK_TRUE
        };

        VkDeviceCreateInfo deviceInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &dynamicRenderingFeatures,
            .flags = 0,
            .queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size()),
            .pQueueCreateInfos = queueInfos.data(),
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
            .pEnabledFeatures = &deviceFeatures
        };

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
            .instance = instance->instance,
            .vulkanApiVersion = VK_API_VERSION_1_3,
        };
        vmaCreateAllocator(&allocatorInfo, &allocator);

        graphicsQueue = getQueueHandle(graphicsQueueFamily.index, 0);

        presentQueue = getQueueHandle(presentQueueFamily.index, 0);

        transferQueue = getQueueHandle(transferQueueFamily.index, 0);
    }

    VulkanDevice::VulkanDevice(VulkanDevice &&other) noexcept
        : gpu(std::move(other.gpu)),
          device(other.device),
          allocator(other.allocator),
          surface(other.surface),
          graphicsQueueFamily(other.graphicsQueueFamily),
          graphicsQueue(other.graphicsQueue),
          presentQueueFamily(other.presentQueueFamily),
          presentQueue(other.presentQueue),
          transferQueueFamily(other.transferQueueFamily),
          transferQueue(other.transferQueue),
          transferCommandPool(other.transferCommandPool) {}

    VulkanDevice &VulkanDevice::operator=(VulkanDevice &&other) noexcept {
        std::swap(gpu, other.gpu);
        std::swap(device, other.device);
        std::swap(allocator, other.allocator);
        std::swap(surface, other.surface);
        std::swap(graphicsQueueFamily, other.graphicsQueueFamily);
        std::swap(graphicsQueue, other.graphicsQueue);
        std::swap(presentQueueFamily, other.presentQueueFamily);
        std::swap(presentQueue, other.presentQueue);
        std::swap(transferQueueFamily, other.presentQueueFamily);
        std::swap(transferQueue, other.transferQueue);
        std::swap(transferCommandPool, other.transferCommandPool);

        return *this;
    }

    VulkanDevice::~VulkanDevice() {
        waitIdle();
        graphicsQueue = nullptr;
        presentQueue = nullptr;
        transferCommandPool = nullptr;
        vmaDestroyAllocator(allocator);
        vkDestroyDevice(device, nullptr);
    }

    void VulkanDevice::waitIdle() const {
        vkDeviceWaitIdle(device);
    }

    VkQueue VulkanDevice::getQueueHandle(uint32_t queueFamilyIndex, uint32_t queueIndex) const {
        VkQueue queue = VK_NULL_HANDLE;
        vkGetDeviceQueue(device, queueFamilyIndex, queueIndex, &queue);
        if (queue == VK_NULL_HANDLE) {
            spdlog::error("Failed to get queue handle for queue family {} and index {}", queueFamilyIndex, queueIndex);
            throw std::runtime_error("Failed to get queue handle");
        }

        return queue;
    }

    VkDevice VulkanDevice::getDevice() const { return device; }

    const GraphicsCard &VulkanDevice::getGpu() const { return gpu; }

    VkSurfaceKHR VulkanDevice::getSurface() const { return surface; }

    const QueueFamily &VulkanDevice::getGraphicsQueueFamily() const { return graphicsQueueFamily; }

    VkQueue VulkanDevice::getGraphicsQueue() const { return graphicsQueue; }

    const QueueFamily &VulkanDevice::getTransferQueueFamily() const { return transferQueueFamily; }

    VkQueue VulkanDevice::getTransferQueue() const { return transferQueue; }

    const QueueFamily &VulkanDevice::getPresentQueueFamily() const { return presentQueueFamily; }

    VkQueue VulkanDevice::getPresentQueue() const { return presentQueue; }

    std::shared_ptr<VulkanCommandPool> VulkanDevice::getTransferCommandPool() {
        if (!transferCommandPool)
            transferCommandPool = allocateCommandPool(CommandPoolUsage::Transient, true);

        return transferCommandPool;
    }

    VmaAllocator VulkanDevice::getAllocator() const { return allocator; }

    std::shared_ptr<VulkanCommandPool> VulkanDevice::allocateCommandPool(
        const CommandPoolUsage usage, const bool createReset) {
        return std::make_shared<VulkanCommandPool>(
            shared_from_this(),
            transferQueueFamily.index,
            usage,
            createReset
        );
    }
}
