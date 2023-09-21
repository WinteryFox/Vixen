#pragma once

#include "Device.h"

namespace Vixen::Vk {
    class VkCommandPool {
    public:
        enum class Usage {
            GRAPHICS,
            TRANSIENT
        };

    private:
        std::shared_ptr<Device> device;

        ::VkCommandPool commandPool;

    public:
        VkCommandPool(const std::shared_ptr<Device> &device, Usage usage);

        VkCommandPool(const VkCommandPool &) = delete;

        VkCommandPool &operator=(const VkCommandPool &) = delete;

        ~VkCommandPool();

        [[nodiscard]] ::VkCommandPool getCommandPool() const;

        [[nodiscard]] const std::shared_ptr<Device> &getDevice() const;

        VkCommandBuffer createCommandBuffers();
    };
}
