#pragma once

#include "Device.h"
#include "VkCommandBuffer.h"

namespace Vixen::Vk {
    class VkCommandPool : public std::enable_shared_from_this<VkCommandPool> {
    public:
        enum class Usage {
            GRAPHICS,
            TRANSIENT
        };

    private:
        std::shared_ptr<Device> device;

        ::VkCommandPool commandPool;

        static std::vector<::VkCommandBuffer> allocateCommandBuffers(
            ::VkDevice device,
            ::VkCommandPool commandPool,
            VkCommandBufferLevel level,
            uint32_t count
        );

    public:
        VkCommandPool(const std::shared_ptr<Device>& device, uint32_t queueFamilyIndex, Usage usage,
                      bool createReset);

        VkCommandPool(const VkCommandPool&) = delete;

        VkCommandPool& operator=(const VkCommandPool&) = delete;

        VkCommandPool(VkCommandPool&& other) noexcept;

        VkCommandPool& operator=(VkCommandPool&& other) noexcept;

        ~VkCommandPool();

        std::vector<VkCommandBuffer> allocate(VkCommandBuffer::Level level, uint32_t count);

        VkCommandBuffer allocate(VkCommandBuffer::Level level);

        void reset() const;

        [[nodiscard]] std::shared_ptr<Device> getDevice() const;

        [[nodiscard]] ::VkCommandPool getCommandPool() const;
    };
}
