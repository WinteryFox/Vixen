#include "VkCommandPool.h"

namespace Vixen::Vk {
    VkCommandPool::VkCommandPool(
            const std::shared_ptr<Device> &device,
            Usage usage
    ) : device(device),
        commandPool(VK_NULL_HANDLE) {
        VkCommandPoolCreateFlags flags;
        uint32_t index;
        switch (usage) {
            case Usage::GRAPHICS:
                flags = 0;
                index = device->getGraphicsQueueFamily().index;
                break;
            case Usage::TRANSIENT:
                flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
                index = device->getTransferQueueFamily().index;
                break;
        }

        VkCommandPoolCreateInfo info{
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .flags = flags,
                .queueFamilyIndex = index
        };

        checkVulkanResult(
                vkCreateCommandPool(device->getDevice(), &info, nullptr, &commandPool),
                "Failed to create command pool"
        );
    }

    VkCommandPool::~VkCommandPool() {
        vkDestroyCommandPool(device->getDevice(), commandPool, nullptr);
    }

    ::VkCommandPool VkCommandPool::getCommandPool() const {
        return commandPool;
    }

    const std::shared_ptr<Device> &VkCommandPool::getDevice() const {
        return device;
    }

//    VkCommandBuffer VkCommandPool::createCommandBuffer() {
//        VkCommandBufferAllocateInfo info{
//                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
//                .commandPool = commandPool,
//                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
//                .commandBufferCount = 1
//        };
//
//        checkVulkanResult(
//                vkAllocateCommandBuffers(device, &info, &commandBuffer),
//                "Failed to create command buffers"
//        );
//    }
}
