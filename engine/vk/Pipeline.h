#pragma once

#include "Device.h"
#include "VkShaderProgram.h"

namespace Vixen::Engine {
    class Pipeline {
    public:
        struct Config {
            VkViewport viewport;
            VkRect2D scissor;
            VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
            VkPipelineRasterizationStateCreateInfo rasterizationInfo;
            VkPipelineMultisampleStateCreateInfo multisampleInfo;
            VkPipelineColorBlendAttachmentState colorBlendAttachment;
            VkPipelineColorBlendStateCreateInfo colorBlendInfo;
            VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
            VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
            VkRenderPass renderPass = VK_NULL_HANDLE;
            uint32_t subpass = 0;
        };

    protected:
        std::shared_ptr<Device> device;

        VkShaderProgram program;

        Config config;

        VkPipeline pipeline = VK_NULL_HANDLE;

    public:
        Pipeline(const std::shared_ptr<Device> &device, const VkShaderProgram &program, const Config &config);

        Pipeline(const Pipeline &) = delete;

        Pipeline &operator=(const Pipeline &) = delete;

        ~Pipeline();

        class Builder {
            uint32_t width = 720;

            uint32_t height = 480;

            Config config{};

        public:
            Builder() {
                config.viewport = {
                        .x = 0,
                        .y = 0,
                        .width = static_cast<float>(width),
                        .height = static_cast<float>(height),
                        .minDepth = 0.0f,
                        .maxDepth = 1.0f
                };
                config.scissor = {
                        .offset{0, 0},
                        .extent{width, height}
                };
                config.inputAssemblyInfo = {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                        .primitiveRestartEnable = VK_FALSE
                };
                config.rasterizationInfo = {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                        .depthClampEnable = VK_FALSE,
                        .rasterizerDiscardEnable = VK_FALSE,
                        .polygonMode = VK_POLYGON_MODE_FILL,
                        .cullMode = VK_CULL_MODE_NONE,
                        .frontFace = VK_FRONT_FACE_CLOCKWISE,
                        .depthBiasEnable = VK_FALSE,
                        .depthBiasConstantFactor = 0.0f,
                        .depthBiasClamp = 0.0f,
                        .depthBiasSlopeFactor = 0.0f,
                        .lineWidth = 1.0f
                };
                config.multisampleInfo = {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
                        .sampleShadingEnable = VK_FALSE,
                        .minSampleShading = 1.0f,
                        .pSampleMask = VK_NULL_HANDLE,
                        .alphaToCoverageEnable = VK_FALSE,
                        .alphaToOneEnable = VK_FALSE
                };
                config.colorBlendAttachment = {
                        .blendEnable = VK_FALSE,
                        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
                        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                        .colorBlendOp = VK_BLEND_OP_ADD,
                        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                        .alphaBlendOp = VK_BLEND_OP_ADD,
                        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
                };
                config.colorBlendInfo = {
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
                config.depthStencilInfo = {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                        .depthTestEnable = VK_TRUE,
                        .depthWriteEnable = VK_TRUE,
                        .depthCompareOp = VK_COMPARE_OP_LESS,
                        .depthBoundsTestEnable = VK_FALSE,
                        .stencilTestEnable = VK_FALSE,
                        .front{},
                        .back{},
                        .minDepthBounds = 0.0f,
                        .maxDepthBounds = 1.0f,
                };
            }

            Builder &setTopology(VkPrimitiveTopology topology) {
                config.inputAssemblyInfo.topology = topology;
                return *this;
            }

            Builder &setPrimitiveRestartEnable(bool enable) {
                config.inputAssemblyInfo.primitiveRestartEnable = enable ? VK_TRUE : VK_FALSE;
                return *this;
            }

            // TODO: Write out rest of builder

            std::unique_ptr<Pipeline> build(
                    const std::shared_ptr<Device> &d,
                    const VkShaderProgram &p
            ) {
                return std::move(std::make_unique<Pipeline>(d, p, config));
            }
        };
    };
}
