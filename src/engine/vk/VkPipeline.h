#pragma once

#include "Device.h"
#include "VkShaderProgram.h"
#include "Swapchain.h"
#include "VkPipelineLayout.h"
#include "VkRenderPass.h"

// TODO: This should really be abstracted so that we can later more easily create different pipelines
namespace Vixen::Vk {
    class VkPipeline {
    public:
        struct Config {
            VkViewport viewport{};
            VkRect2D scissor{};
            VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
            VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
            VkPipelineMultisampleStateCreateInfo multisampleInfo{};
            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
            uint32_t subpass = 0;
        };

    protected:
        std::shared_ptr<Device> device;

        VkShaderProgram program;

        Config config;

        VkRenderPass renderPass;

        VkPipelineLayout pipelineLayout;

        ::VkPipeline pipeline = VK_NULL_HANDLE;

    public:
        VkPipeline(const std::shared_ptr<Device> &device, const Swapchain &swapchain, const VkShaderProgram &program,
                   const Config &config);

        VkPipeline(VkPipeline& other) = delete;

        VkPipeline& operator=(const VkPipeline& other) = delete;

        VkPipeline(VkPipeline&& other) noexcept;

        VkPipeline const& operator=(VkPipeline&& other) noexcept;

        ~VkPipeline();

        void bind(::VkCommandBuffer commandBuffer, VkPipelineBindPoint binding) const;

        void bindGraphics(::VkCommandBuffer commandBuffer) const;

        void bindCompute(::VkCommandBuffer commandBuffer) const;

        void bindRayTracing(::VkCommandBuffer commandBuffer) const;

        [[nodiscard]] const VkShaderProgram &getProgram() const;

        [[nodiscard]] const VkRenderPass &getRenderPass() const;

        [[nodiscard]] const Config &getConfig() const;

        class Builder {
            Config config{
                    .viewport = {
                            .x = 0,
                            .y = 0,
                            .minDepth = 0.0f,
                            .maxDepth = 1.0f
                    },
                    .scissor = {
                            .offset = {0, 0},
                    }
            };

        public:
            Builder() {
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
                        .cullMode = VK_CULL_MODE_BACK_BIT,
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
                config.depthStencilInfo = {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                        .depthTestEnable = VK_TRUE,
                        .depthWriteEnable = VK_TRUE,
                        .depthCompareOp = VK_COMPARE_OP_LESS,
                        .depthBoundsTestEnable = VK_FALSE,
                        .stencilTestEnable = VK_FALSE,
                        .front = {},
                        .back = {},
                        .minDepthBounds = 0.0f,
                        .maxDepthBounds = 1.0f,
                };
            }

            Builder &setWidth(const uint32_t w) {
                config.viewport.width = static_cast<float>(w);
                config.scissor.extent.width = w;
                return *this;
            }

            Builder &setHeight(const int32_t h) {
                config.viewport.height = static_cast<float>(-h);
                config.viewport.y = static_cast<float>(h);
                config.scissor.extent.height = h;
                return *this;
            }

            Builder &setInputAssembly(VkPipelineInputAssemblyStateCreateInfo info) {
                config.inputAssemblyInfo = info;
                return *this;
            }

            Builder &setRasterization(VkPipelineRasterizationStateCreateInfo info) {
                config.rasterizationInfo = info;
                return *this;
            }

            Builder &setMultisample(VkPipelineMultisampleStateCreateInfo info) {
                config.multisampleInfo = info;
                return *this;
            }

            Builder &setColorBlend(VkPipelineColorBlendAttachmentState info) {
                config.colorBlendAttachment = info;
                return *this;
            }

            Builder &setDepthStencil(VkPipelineDepthStencilStateCreateInfo info) {
                config.depthStencilInfo = info;
                return *this;
            }

            std::shared_ptr<VkPipeline> build(
                    const std::shared_ptr<Device> &d,
                    const Swapchain &s,
                    const VkShaderProgram &p
            ) {
                return std::make_shared<VkPipeline>(d, s, p, config);
            }
        };
    };
}
