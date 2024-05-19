#pragma once

#include <vector>

#include "VulkanDescriptorSet.h"

namespace Vixen {
    class VulkanDevice;

    class VulkanDescriptorSetLayout {
        VkDescriptorSetLayout layout;

        std::shared_ptr<VulkanDevice> device;

    public:
        VulkanDescriptorSetLayout(
            const std::shared_ptr<VulkanDevice>& device,
            const std::vector<VkDescriptorSetLayoutBinding>& bindings
        );

        VulkanDescriptorSetLayout(VulkanDescriptorSetLayout& other) = delete;

        VulkanDescriptorSetLayout& operator=(const VulkanDescriptorSetLayout& other) = delete;

        VulkanDescriptorSetLayout(VulkanDescriptorSetLayout&& fp) noexcept;

        VulkanDescriptorSetLayout const& operator=(VulkanDescriptorSetLayout&& fp) noexcept;

        ~VulkanDescriptorSetLayout();

        [[nodiscard]] ::VkDescriptorSetLayout getLayout() const;
    };
}
