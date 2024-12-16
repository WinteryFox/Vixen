#include "VulkanPipeline.h"

#include "device/VulkanDevice.h"
#include "shader/VulkanShaderModule.h"

namespace Vixen {
    VulkanPipeline::VulkanPipeline(
        const std::shared_ptr<VulkanDevice> &device,
        const VulkanShaderProgram &program,
        const Config &config
    ) : device(device),
        program(program),
        config(config),
        pipelineLayout(device, program) {
        std::vector<VkPipelineShaderStageCreateInfo> stages;
        stages.emplace_back(program.getVertex()->createInfo());
        stages.emplace_back(program.getFragment()->createInfo());

        constexpr std::array dynamicStates{
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicStateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
            .pDynamicStates = dynamicStates.data()
        };

        std::vector<VkVertexInputBindingDescription> vertexBindings{};
        for (const auto &[binding, stride, rate] : config.bindings) {
            vertexBindings.push_back(
                {
                    .binding = binding,
                    .stride = static_cast<uint32_t>(stride),
                    .inputRate = toVkVertexInputRate(rate)
                }
            );
        }

        std::vector<VkVertexInputAttributeDescription> vertexAttributes{};
        for (const auto &[binding, location, type, offset]: config.inputs) {
            VkFormat f = VK_FORMAT_MAX_ENUM;
            switch (type) {
                    using enum ShaderResources::PrimitiveType;

                case Float1:
                    f = VK_FORMAT_R32_SFLOAT;
                    break;

                case Float2:
                    f = VK_FORMAT_R32G32_SFLOAT;
                    break;

                case Float3:
                    f = VK_FORMAT_R32G32B32_SFLOAT;
                    break;

                case Float4:
                    f = VK_FORMAT_R32G32B32A32_SFLOAT;
                    break;
            }

            vertexAttributes.push_back(
                {
                    .location = location,
                    .binding = binding,
                    .format = f,
                    .offset = static_cast<uint32_t>(offset)
                }
            );
        }

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindings.size()),
            .pVertexBindingDescriptions = vertexBindings.data(),
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size()),
            .pVertexAttributeDescriptions = vertexAttributes.data(),
        };

        VkPipelineViewportStateCreateInfo viewportInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .viewportCount = 1,
            .pViewports = &config.viewport,
            .scissorCount = 1,
            .pScissors = &config.scissor
        };

        VkPipelineColorBlendStateCreateInfo colorBlendInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
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

        const VkPipelineRenderingCreateInfo renderingCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .pNext = nullptr,
            .viewMask = 0,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &config.colorFormat,
            .depthAttachmentFormat = config.depthFormat,
            .stencilAttachmentFormat = config.depthFormat
        };

        const VkGraphicsPipelineCreateInfo pipelineCreateInfo{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &renderingCreateInfo,
            .flags = 0,
            .stageCount = static_cast<uint32_t>(stages.size()),
            .pStages = stages.data(),
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &config.inputAssemblyInfo,
            .pTessellationState = nullptr,
            .pViewportState = &viewportInfo,
            .pRasterizationState = &config.rasterizationInfo,
            .pMultisampleState = &config.multisampleInfo,
            .pDepthStencilState = &config.depthStencilInfo,
            .pColorBlendState = &colorBlendInfo,
            .pDynamicState = &dynamicStateInfo,

            .layout = pipelineLayout.getLayout(),
            .renderPass = VK_NULL_HANDLE,
            .subpass = config.subpass,

            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1
        };

        checkVulkanResult(
            vkCreateGraphicsPipelines(device->getDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline),
            "Failed to create Vulkan pipeline"
        );
    }

    VulkanPipeline::VulkanPipeline(VulkanPipeline &&other) noexcept
        : device(std::exchange(other.device, nullptr)),
          program(std::move(other.program)),
          config(other.config),
          pipelineLayout(std::move(other.pipelineLayout)),
          pipeline(std::exchange(other.pipeline, nullptr)) {}

    VulkanPipeline &VulkanPipeline::operator=(VulkanPipeline &&other) noexcept {
        std::swap(device, other.device);
        std::swap(program, other.program);
        std::swap(config, other.config);
        std::swap(pipelineLayout, other.pipelineLayout);
        std::swap(pipeline, other.pipeline);

        return *this;
    }

    VulkanPipeline::~VulkanPipeline() {
        vkDestroyPipeline(device->getDevice(), pipeline, nullptr);
    }

    const VulkanShaderProgram &VulkanPipeline::getProgram() const { return program; }

    const VulkanPipeline::Config &VulkanPipeline::getConfig() const { return config; }

    std::shared_ptr<VulkanDevice> VulkanPipeline::getDevice() const { return device; }

    VkPipeline VulkanPipeline::getPipeline() const { return pipeline; }

    const VulkanPipelineLayout &VulkanPipeline::getLayout() const { return pipelineLayout; }
}
