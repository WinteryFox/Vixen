#pragma once

#include "Device.h"

namespace Vixen::Vk {
    class VkDescriptorSetLayout {
        ::VkDescriptorSetLayout layout;

        std::shared_ptr<Device> device;

    public:
        VkDescriptorSetLayout(
            const std::shared_ptr<Device>& device,
            const std::vector<VkDescriptorSetLayoutBinding>& bindings
        );

        VkDescriptorSetLayout(VkDescriptorSetLayout& other) = delete;

        VkDescriptorSetLayout& operator=(const VkDescriptorSetLayout& other) = delete;

        VkDescriptorSetLayout(VkDescriptorSetLayout&& fp) noexcept;

        VkDescriptorSetLayout const& operator=(VkDescriptorSetLayout&& fp) noexcept;

        ~VkDescriptorSetLayout();

        [[nodiscard]] ::VkDescriptorSetLayout getLayout() const;
    };
}
