#include "VulkanPipelineLayout.h"

#include "device/VulkanDevice.h"
#include "descriptorset/VulkanDescriptorSetLayout.h"
#include "shader/VulkanShaderProgram.h"

namespace Vixen {
    VulkanPipelineLayout::VulkanPipelineLayout(const std::shared_ptr<VulkanDevice>& device, const VulkanShaderProgram& program)
        : device(device) {
        std::vector<::VkDescriptorSetLayout> layouts;
        layouts.push_back(program.getDescriptorSetLayout()->getLayout());
        const VkPipelineLayoutCreateInfo pipelineLayoutInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = static_cast<uint32_t>(layouts.size()),
            .pSetLayouts = layouts.data(),
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr
        };

        checkVulkanResult(
            vkCreatePipelineLayout(device->getDevice(), &pipelineLayoutInfo, nullptr, &layout),
            "Failed to create pipeline layout"
        );
    }

    VulkanPipelineLayout::VulkanPipelineLayout(VulkanPipelineLayout&& fp) noexcept
        : device(std::move(device)),
          layout(std::exchange(fp.layout, nullptr)) {}

    VulkanPipelineLayout& VulkanPipelineLayout::operator=(VulkanPipelineLayout&& fp) noexcept {
        std::swap(device, fp.device);
        std::swap(layout, fp.layout);

        return *this;
    }

    VulkanPipelineLayout::~VulkanPipelineLayout() {
        vkDestroyPipelineLayout(device->getDevice(), layout, nullptr);
    }

    ::VkPipelineLayout VulkanPipelineLayout::getLayout() const {
        return layout;
    }
}
