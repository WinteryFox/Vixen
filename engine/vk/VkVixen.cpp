#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VOLK_IMPLEMENTATION

#include "VkVixen.h"

namespace Vixen::Engine {
    VkVixen::VkVixen(const std::string &appTitle)
            : window(VkWindow(appTitle, 720, 480, false)),
              instance(Instance(appTitle, glm::vec3(1, 0, 0), window.requiredExtensions)),
              device(Device(
                      deviceExtensions,
                      instance.findOptimalGraphicsCard(deviceExtensions),
                      instance.surfaceForWindow(window)
              )),
              allocator(VK_NULL_HANDLE) {
        VmaVulkanFunctions vulkanFunctions{
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
                .physicalDevice = device.gpu.device,
                .device = device.device,
                .pVulkanFunctions = &vulkanFunctions,
                .instance = instance.instance,
                .vulkanApiVersion = VK_API_VERSION_1_3,
        };
        vmaCreateAllocator(&allocatorInfo, &allocator);

        window.center();
        window.setVisible(true);
    }

    VkVixen::~VkVixen() {
        vmaDestroyAllocator(allocator);
    }
}
