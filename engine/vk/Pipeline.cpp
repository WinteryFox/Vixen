#include "Pipeline.h"

namespace Vixen::Vk {
    Pipeline::Pipeline(
            const std::shared_ptr<Device> &device,
            const Swapchain &swapchain,
            const VkShaderProgram &program,
            const Config &config
    ) : device(device), program(program), config(config) {
        const auto &modules = program.getModules();

        std::vector<VkPipelineShaderStageCreateInfo> stages;
        stages.reserve(modules.size());
        for (const auto &module: modules) {
            stages.emplace_back(VkPipelineShaderStageCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = VK_NULL_HANDLE,
                    .flags = 0,
                    .stage = getVulkanShaderStage(module->getStage()),
                    .module = module->getModule(),
                    .pName = module->getEntrypoint().c_str(),
                    .pSpecializationInfo = VK_NULL_HANDLE
            });
        }

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .vertexBindingDescriptionCount = 0,
                .pVertexBindingDescriptions = nullptr,
                .vertexAttributeDescriptionCount = 0,
                .pVertexAttributeDescriptions = nullptr,
        };

        VkPipelineViewportStateCreateInfo viewportInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .viewportCount = 1,
                .pViewports = &config.viewport,
                .scissorCount = 1,
                .pScissors = &config.scissor
        };

        VkPipelineColorBlendStateCreateInfo colorBlendInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .logicOpEnable = VK_FALSE,
                .logicOp = VK_LOGIC_OP_COPY,
                .attachmentCount = 1,
                .pAttachments = &config.colorBlendAttachment,
                .blendConstants{
                        0.0f,
                        0.0f,
                        0.0f,
                        0.0f
                }
        };

        assert(
                config.pipelineLayout != VK_NULL_HANDLE &&
                "Pipeline config has a layout of null"
        );
        assert(
                config.renderPass != VK_NULL_HANDLE &&
                "Pipeline config has a render pass of null"
        );
        VkGraphicsPipelineCreateInfo pipelineInfo{
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .stageCount = static_cast<uint32_t>(stages.size()),
                .pStages = stages.data(),
                .pVertexInputState = &vertexInputInfo,
                .pInputAssemblyState = &config.inputAssemblyInfo,
                .pViewportState = &viewportInfo,
                .pRasterizationState = &config.rasterizationInfo,
                .pMultisampleState = &config.multisampleInfo,
                .pDepthStencilState= &config.depthStencilInfo,
                .pColorBlendState = &colorBlendInfo,
                .pDynamicState= nullptr,

                .layout = config.pipelineLayout,
                .renderPass = config.renderPass,
                .subpass = config.subpass,

                .basePipelineHandle = VK_NULL_HANDLE,
                .basePipelineIndex = -1,
        };

        checkVulkanResult(
                vkCreateGraphicsPipelines(device->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline),
                "Failed to create Vulkan pipeline"
        );
    }

    Pipeline::~Pipeline() {
        vkDestroyPipeline(device->getDevice(), pipeline, nullptr);
    }
}
