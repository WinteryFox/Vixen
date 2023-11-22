#include "VkDescriptorSetLayout.h"

namespace Vixen::Vk {
    VkDescriptorSetLayout::VkDescriptorSetLayout(
        const std::shared_ptr<Device>& device,
        const std::vector<VkDescriptorSetLayoutBinding>& bindings
    ) : layout(VK_NULL_HANDLE) {
        const VkDescriptorSetLayoutCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data()
        };

        checkVulkanResult(
            vkCreateDescriptorSetLayout(device->getDevice(), &info, nullptr, &layout),
            "Failed to create descriptor set layout"
        );
    }

    VkDescriptorSetLayout::VkDescriptorSetLayout(VkDescriptorSetLayout&& fp) noexcept
        : layout(std::exchange(fp.layout, nullptr)),
          device(std::move(fp.device)) {}

    VkDescriptorSetLayout const& VkDescriptorSetLayout::operator=(VkDescriptorSetLayout&& fp) noexcept {
        std::swap(fp.layout, layout);

        return *this;
    }

    VkDescriptorSetLayout::~VkDescriptorSetLayout() {
        vkDestroyDescriptorSetLayout(device->getDevice(), layout, nullptr);
    }

    ::VkDescriptorSetLayout VkDescriptorSetLayout::getLayout() const {
        return layout;
    }
}
