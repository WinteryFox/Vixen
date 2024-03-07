#include "VkPipelineLayout.h"

namespace Vixen::Vk {
    VkPipelineLayout::VkPipelineLayout(const std::shared_ptr<Device>& device, const VkShaderProgram& program)
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

    VkPipelineLayout::VkPipelineLayout(VkPipelineLayout&& fp) noexcept
        : device(std::move(device)),
          layout(std::exchange(fp.layout, nullptr)) {}

    VkPipelineLayout& VkPipelineLayout::operator=(VkPipelineLayout&& fp) noexcept {
        std::swap(device, fp.device);
        std::swap(layout, fp.layout);

        return *this;
    }

    VkPipelineLayout::~VkPipelineLayout() {
        vkDestroyPipelineLayout(device->getDevice(), layout, nullptr);
    }

    ::VkPipelineLayout VkPipelineLayout::getLayout() const {
        return layout;
    }
}
