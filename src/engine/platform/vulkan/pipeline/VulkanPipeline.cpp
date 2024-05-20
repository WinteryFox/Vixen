#include "VulkanPipeline.h"

#include "device/VulkanDevice.h"
#include "shader/VulkanShaderModule.h"

namespace Vixen {
    VulkanPipeline::VulkanPipeline(
        const std::shared_ptr<VulkanDevice>& device,
        const VulkanShaderProgram& program,
        const Config& config
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

        const auto& vertexModule = program.getVertex();

        std::vector<VkVertexInputBindingDescription> vertexBindings{};
        for (const auto& [binding, stride, rate] : vertexModule->getBindings()) {
            VkVertexInputRate r;
            switch (rate) {
            case VulkanShaderModule::Rate::Vertex:
                r = VK_VERTEX_INPUT_RATE_VERTEX;
                break;
            case VulkanShaderModule::Rate::Instance:
                r = VK_VERTEX_INPUT_RATE_INSTANCE;
                break;
            default:
                throw std::runtime_error("Not implemented input rate");
            }

            vertexBindings.push_back(
                {
                    .binding = binding,
                    .stride = static_cast<uint32_t>(stride),
                    .inputRate = r
                }
            );
        }

        std::vector<VkVertexInputAttributeDescription> vertexAttributes{};
        for (const auto& [binding, location, size, offset] : vertexModule->getInputs()) {
            VkFormat format;
            switch (size) {
            case 2 * sizeof(float):
                format = VK_FORMAT_R32G32_SFLOAT;
                break;
            case 3 * sizeof(float):
                format = VK_FORMAT_R32G32B32_SFLOAT;
                break;
            case 4 * sizeof(float):
                format = VK_FORMAT_R32G32B32A32_SFLOAT;
                break;
            default:
                throw std::runtime_error("Unsupported input format");
            }

            vertexAttributes.push_back(
                {
                    .location = location.value_or(0),
                    .binding = binding.value_or(0),
                    // TODO: Automatically determine the format and offset
                    .format = format,
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

    VulkanPipeline::VulkanPipeline(VulkanPipeline&& other) noexcept
        : device(std::exchange(other.device, nullptr)),
          program(std::move(other.program)),
          config(other.config),
          pipelineLayout(std::move(other.pipelineLayout)),
          pipeline(std::exchange(other.pipeline, nullptr)) {}

    VulkanPipeline& VulkanPipeline::operator=(VulkanPipeline&& other) noexcept {
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

    void VulkanPipeline::bind(::VkCommandBuffer commandBuffer, VkPipelineBindPoint binding) const {
        vkCmdBindPipeline(commandBuffer, binding, pipeline);
    }

    void VulkanPipeline::bindGraphics(::VkCommandBuffer commandBuffer) const {
        bind(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
    }

    void VulkanPipeline::bindCompute(::VkCommandBuffer commandBuffer) const {
        bind(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE);
    }

    void VulkanPipeline::bindRayTracing(::VkCommandBuffer commandBuffer) const {
        bind(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR);
    }

    const VulkanShaderProgram& VulkanPipeline::getProgram() const { return program; }

    const VulkanPipeline::Config& VulkanPipeline::getConfig() const { return config; }

    std::shared_ptr<VulkanDevice> VulkanPipeline::getDevice() const { return device; }

    const VulkanPipelineLayout& VulkanPipeline::getLayout() const { return pipelineLayout; }
}
