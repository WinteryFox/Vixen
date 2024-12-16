#include "VulkanPipelineLayout.h"

#include "device/VulkanDevice.h"
#include "descriptorset/VulkanDescriptorSetLayout.h"
#include "shader/VulkanShaderProgram.h"

namespace Vixen {
    VulkanPipelineLayout::VulkanPipelineLayout(const std::shared_ptr<VulkanDevice> &device,
                                               const VulkanShaderProgram &program)
        : device(device),
          layout(VK_NULL_HANDLE) {
        std::vector<::VkDescriptorSetLayout> layouts;
        layouts.push_back(program.getDescriptorSetLayout()->getLayout());
        const auto &pushConstants = program.getVertex()->getResources().pushConstants;
        std::vector<VkPushConstantRange> ranges{pushConstants.size()};
        for (uint32_t i = 0; i < ranges.size(); i++) {
            const auto &[stage, offset, size] = pushConstants[i];

            ranges[i] = {
                .stageFlags = toVkShaderStage(stage),
                .offset = offset,
                .size = size
            };
        }

        const VkPipelineLayoutCreateInfo pipelineLayoutInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = static_cast<uint32_t>(layouts.size()),
            .pSetLayouts = layouts.data(),
            .pushConstantRangeCount = static_cast<uint32_t>(ranges.size()),
            .pPushConstantRanges = ranges.data()
        };

        checkVulkanResult(
            vkCreatePipelineLayout(device->getDevice(), &pipelineLayoutInfo, nullptr, &layout),
            "Failed to create pipeline layout"
        );
    }

    VulkanPipelineLayout::VulkanPipelineLayout(VulkanPipelineLayout &&fp) noexcept
        : device(std::exchange(fp.device, nullptr)),
          layout(std::exchange(fp.layout, nullptr)) {}

    VulkanPipelineLayout &VulkanPipelineLayout::operator=(VulkanPipelineLayout &&fp) noexcept {
        std::swap(device, fp.device);
        std::swap(layout, fp.layout);

        return *this;
    }

    VulkanPipelineLayout::~VulkanPipelineLayout() {
        if (device)
            vkDestroyPipelineLayout(device->getDevice(), layout, nullptr);
    }

    VkPipelineLayout VulkanPipelineLayout::getLayout() const {
        return layout;
    }
}
