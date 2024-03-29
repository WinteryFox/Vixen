#include "VkPipeline.h"

namespace Vixen::Vk {
    VkPipeline::VkPipeline(
        const std::shared_ptr<Device>& device,
        const VkShaderProgram& program,
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
            .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
            .pDynamicStates = dynamicStates.data()
        };

        const auto& vertexModule = program.getVertex();

        std::vector<VkVertexInputBindingDescription> vertexBindings{};
        for (const auto& [binding, stride, rate] : vertexModule->getBindings()) {
            VkVertexInputRate r;
            switch (rate) {
            case ShaderModule::Rate::Vertex:
                r = VK_VERTEX_INPUT_RATE_VERTEX;
                break;
            case ShaderModule::Rate::Instance:
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
            .vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindings.size()),
            .pVertexBindingDescriptions = vertexBindings.data(),
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size()),
            .pVertexAttributeDescriptions = vertexAttributes.data(),
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
            .stageCount = static_cast<uint32_t>(stages.size()),
            .pStages = stages.data(),
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &config.inputAssemblyInfo,
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

    VkPipeline::VkPipeline(VkPipeline&& other) noexcept
        : device(std::move(other.device)),
          program(std::move(other.program)),
          config(other.config),
          pipelineLayout(std::move(other.pipelineLayout)),
          pipeline(std::exchange(other.pipeline, nullptr)) {}

    VkPipeline& VkPipeline::operator=(VkPipeline&& other) noexcept {
        std::swap(device, other.device);
        std::swap(program, other.program);
        std::swap(config, other.config);
        std::swap(pipelineLayout, other.pipelineLayout);
        std::swap(pipeline, other.pipeline);

        return *this;
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

    const VkShaderProgram& VkPipeline::getProgram() const { return program; }

    const VkPipeline::Config& VkPipeline::getConfig() const { return config; }

    std::shared_ptr<Device> VkPipeline::getDevice() const { return device; }

    const VkPipelineLayout& VkPipeline::getLayout() const { return pipelineLayout; }
}
