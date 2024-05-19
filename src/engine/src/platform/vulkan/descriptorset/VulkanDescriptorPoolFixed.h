#pragma once

#include "VulkanDescriptorSet.h"
#include "VulkanDescriptorSetLayout.h"

namespace Vixen {
    class VulkanDescriptorPoolFixed : public std::enable_shared_from_this<VulkanDescriptorPoolFixed> {
        std::shared_ptr<VulkanDevice> device;

        ::VkDescriptorPool pool;

    public:
        VulkanDescriptorPoolFixed(
            const std::shared_ptr<VulkanDevice>& device,
            const std::vector<VkDescriptorPoolSize>& sizes,
            uint32_t maxSets
        );

        VulkanDescriptorPoolFixed(VulkanDescriptorPoolFixed& other) = delete;

        VulkanDescriptorPoolFixed& operator=(const VulkanDescriptorPoolFixed& other) = delete;

        VulkanDescriptorPoolFixed(VulkanDescriptorPoolFixed&& other) noexcept;

        VulkanDescriptorPoolFixed& operator=(VulkanDescriptorPoolFixed&& other) noexcept;

        ~VulkanDescriptorPoolFixed();

        [[nodiscard]] VulkanDescriptorSet allocate(const VulkanDescriptorSetLayout &layout) const;

        void reset() const;

        [[nodiscard]] std::shared_ptr<VulkanDevice> getDevice() const;

        [[nodiscard]] ::VkDescriptorPool getPool() const;
    };
}
