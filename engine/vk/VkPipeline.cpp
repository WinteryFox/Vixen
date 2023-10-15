#include "VkPipeline.h"

namespace Vixen::Vk {
    VkPipeline::VkPipeline(
            const std::shared_ptr<Device> &device,
            const Swapchain &swapchain,
            const VkShaderProgram &program,
            const Config &config
    ) : device(device),
        program(program),
        config(config),
        pipelineLayout(device, program),
        renderPass(device, program, swapchain) {
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

        std::vector<VkDynamicState> dynamicStates{
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicStateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
            .pDynamicStates = dynamicStates.data()
        };

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
                .blendConstants = {
                        0.0f,
                        0.0f,
                        0.0f,
                        0.0f
                }
        };

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
                .pDynamicState = &dynamicStateInfo,

                .layout = pipelineLayout.getLayout(),
                .renderPass = renderPass.getRenderPass(),
                .subpass = config.subpass,

                .basePipelineHandle = VK_NULL_HANDLE,
                .basePipelineIndex = -1,
        };

        checkVulkanResult(
                vkCreateGraphicsPipelines(device->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline),
                "Failed to create Vulkan pipeline"
        );
    }

    VkPipeline::~VkPipeline() {
        vkDestroyPipeline(device->getDevice(), pipeline, nullptr);
    }

    void VkPipeline::bind(::VkCommandBuffer commandBuffer, VkPipelineBindPoint binding) const {
        vkCmdBindPipeline(commandBuffer, binding, pipeline);
    }

    void VkPipeline::bindGraphics(::VkCommandBuffer commandBuffer) const {
        bind(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
    }

    void VkPipeline::bindCompute(::VkCommandBuffer commandBuffer) const {
        bind(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE);
    }

    void VkPipeline::bindRayTracing(::VkCommandBuffer commandBuffer) const {
        bind(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR);
    }

    const VkShaderProgram &VkPipeline::getProgram() const {
        return program;
    }

    const VkRenderPass &VkPipeline::getRenderPass() const {
        return renderPass;
    }
}
