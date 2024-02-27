#pragma once

#include "VkDescriptorSet.h"
#include "VkDescriptorSetLayout.h"

namespace Vixen::Vk {
    class VkDescriptorPoolFixed : public std::enable_shared_from_this<VkDescriptorPoolFixed> {
        std::shared_ptr<Device> device;

        ::VkDescriptorPool pool;

    public:
        VkDescriptorPoolFixed(
            const std::shared_ptr<Device>& device,
            const std::vector<VkDescriptorPoolSize>& sizes,
            uint32_t maxSets
        );

        VkDescriptorPoolFixed(VkDescriptorPoolFixed& other) = delete;

        VkDescriptorPoolFixed& operator=(const VkDescriptorPoolFixed& other) = delete;

        VkDescriptorPoolFixed(VkDescriptorPoolFixed&& other) noexcept;

        VkDescriptorPoolFixed& operator=(VkDescriptorPoolFixed&& other) noexcept;

        ~VkDescriptorPoolFixed();

        [[nodiscard]] VkDescriptorSet allocate(const VkDescriptorSetLayout &layout) const;

        void reset() const;

        [[nodiscard]] std::shared_ptr<Device> getDevice() const;

        [[nodiscard]] ::VkDescriptorPool getPool() const;
    };
}
