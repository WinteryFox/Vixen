#include "Allocator.h"

namespace Vixen::Engine {
    Allocator::Allocator(VkPhysicalDevice gpu, VkDevice device, VkInstance instance)
            : allocator(VK_NULL_HANDLE) {
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
                .physicalDevice = gpu,
                .device = device,
                .pVulkanFunctions = &vulkanFunctions,
                .instance = instance,
                .vulkanApiVersion = VK_API_VERSION_1_3,
        };
        vmaCreateAllocator(&allocatorInfo, &allocator);
    }

    Allocator::~Allocator() {
        vmaDestroyAllocator(allocator);
    }
}
