#include "VulkanDescriptorSetLayout.h"

#include "Vulkan.h"
#include "VulkanDevice.h"

namespace Vixen {
    VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(
        const std::shared_ptr<VulkanDevice> &device,
        const std::vector<VkDescriptorSetLayoutBinding> &bindings
    ) : layout(VK_NULL_HANDLE),
        device(device) {
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

    VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanDescriptorSetLayout &&fp) noexcept
        : layout(std::exchange(fp.layout, nullptr)),
          device(std::move(fp.device)) {}

    VulkanDescriptorSetLayout &VulkanDescriptorSetLayout::operator=(VulkanDescriptorSetLayout &&fp) noexcept {
        std::swap(fp.layout, layout);
        std::swap(fp.device, device);

        return *this;
    }

    VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout() {
        vkDestroyDescriptorSetLayout(device->getDevice(), layout, nullptr);
    }

    ::VkDescriptorSetLayout VulkanDescriptorSetLayout::getLayout() const {
        return layout;
    }
}
