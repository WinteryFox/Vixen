#pragma once

#ifdef DEBUG

#include <glslang/SPIRV/disassemble.h>

#endif

#include <fstream>
#include <spirv_reflect.hpp>
#include <sstream>
#include <core/exception/ShaderLinkFailedException.h>
#include <core/shader/ShaderModule.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <Volk/volk.h>

#define VIXEN_VK_SPIRV_VERSION 130

namespace Vixen {
    class VulkanDevice;

    class VulkanShaderModule final : public ShaderModule {
        VkShaderModule module;

        std::shared_ptr<VulkanDevice> device;

    public:
        VulkanShaderModule(
            const std::shared_ptr<VulkanDevice> &device,
            ShaderResources::Stage stage,
            const std::vector<uint32_t> &binary,
            const std::string &entrypoint,
            const ShaderResources &resources
        );

        VulkanShaderModule(const VulkanShaderModule &) = delete;

        VulkanShaderModule &operator=(const VulkanShaderModule &) = delete;

        VulkanShaderModule(VulkanShaderModule &&other) noexcept;

        VulkanShaderModule &operator=(VulkanShaderModule &&other) noexcept;

        ~VulkanShaderModule() override;

        [[nodiscard]] VkPipelineShaderStageCreateInfo createInfo() const;

        [[nodiscard]] std::vector<VkDescriptorSetLayoutBinding> createBindings() const;

        [[nodiscard]] std::shared_ptr<VulkanDevice> getDevice() const;

        class Builder {
            // TODO: This builder has slightly confusing API, you can compile before setting the stage meaning its possible to mistakenly have the wrong stage set at compile time
            ShaderResources::Stage stage;

            std::vector<uint32_t> binary{};

            std::string entrypoint = "main";

            ShaderResources resources;

        public:
            explicit Builder(const ShaderResources::Stage stage) : stage(stage) {}

            Builder &setStage(const ShaderResources::Stage s) {
                stage = s;
                return *this;
            }

            Builder &setBinary(const std::vector<uint32_t> &data) {
                binary = data;
                return *this;
            }

            Builder &setEntrypoint(const std::string &entry) {
                entrypoint = entry;
                return *this;
            }

            std::shared_ptr<VulkanShaderModule> compile(const std::shared_ptr<VulkanDevice> &d,
                                                        const std::vector<char> &source) {
                EShLanguage s;
                switch (stage) {
                    case ShaderResources::Stage::Vertex:
                        s = EShLangVertex;
                        break;
                    case ShaderResources::Stage::Fragment:
                        s = EShLangFragment;
                        break;
                    default:
                        spdlog::error("Unsupported stage for shader module");
                        throw std::runtime_error("Unsupported stage for shader module");
                }

                spdlog::trace("Passed in shader source\n{}", source.data());

                glslang::InitializeProcess();
                glslang::TShader shader{s};
                glslang::TProgram program;

                auto src = source.data();
                shader.setStrings(&src, 1);
                shader.setEnvInput(glslang::EShSourceGlsl, s, glslang::EShClientVulkan, VIXEN_VK_SPIRV_VERSION);
                shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
                shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);
                shader.setAutoMapLocations(true);
                shader.setAutoMapBindings(true);

                shader.setEntryPoint(entrypoint.c_str());
                shader.setSourceEntryPoint(entrypoint.c_str());

                auto messages = EShMsgEnhanced;
                // TODO: Add actual includer
                glslang::TShader::ForbidIncluder includer;

                // TODO: Resource limit should probably be gotten from the GPU
                if (!shader.parse(GetDefaultResources(), VIXEN_VK_SPIRV_VERSION, false, messages))
                    throw ShaderLinkFailedException(shader.getInfoLog());

                program.addShader(&shader);
                if (!program.link(messages))
                    throw ShaderLinkFailedException(shader.getInfoLog());

                glslang::SpvOptions options{
#ifdef DEBUG
                    .generateDebugInfo = true,
                    .stripDebugInfo = false,
                    .disableOptimizer = true,
                    .optimizeSize = false,
                    .disassemble = true,
#else
                    .generateDebugInfo = false,
                    .stripDebugInfo = true,
                    .disableOptimizer = false,
                    .optimizeSize = true,
                    .disassemble = false,
#endif
                    .validate = true,
                };

                spv::SpvBuildLogger logger;
                GlslangToSpv(*program.getIntermediate(s), binary, &logger, &options);

#ifdef DEBUG
                std::stringstream stream;
                spv::Disassemble(stream, binary);
                spdlog::debug("Passed in GLSL source string:\n{}\n\nDisassembled SPIR-V:\n{}",
                              std::string_view(source.begin(), source.end()), stream.str());
#endif

                spirv_cross::CompilerReflection c{binary};

                auto reflectedResources = c.get_shader_resources();

                for (const auto &pushConstant: reflectedResources.push_constant_buffers) {
                    // TODO: Determine type of push constant
                    resources.pushConstants.push_back({
                        .stage = stage,
                        .offset = 0,
                        // TODO: Calculate size automatically from type
                        .size = 64
                    });
                }

                for (const auto &[id, type_id, base_type_id, name]: reflectedResources.uniform_buffers) {
                    uint32_t binding = c.get_decoration(id, spv::DecorationBinding);

                    resources.uniforms.push_back({
                        .binding = binding,
                        .type = ShaderResources::UniformType::Buffer
                    });
                }

                for (const auto &[id, type_id, base_type_id, name]: reflectedResources.sampled_images) {
                    uint32_t binding = c.get_decoration(id, spv::DecorationBinding);

                    resources.uniforms.push_back({
                        .binding = binding,
                        .type = ShaderResources::UniformType::Sampler
                    });
                }

                if (!logger.getAllMessages().empty())
                    spdlog::warn("Error messages were generated during shader module compilation;\n{}",
                                 logger.getAllMessages());
                spdlog::trace("Compiled shader to SPIR-V binary {}", spdlog::to_hex(binary.begin(), binary.end()));

                glslang::FinalizeProcess();

                return std::make_shared<VulkanShaderModule>(d, stage, binary, entrypoint, resources);
            }

            std::shared_ptr<VulkanShaderModule> compile(const std::shared_ptr<VulkanDevice> &d,
                                                        const std::string &source) {
                return compile(d, std::vector(source.begin(), source.end()));
            }

            std::shared_ptr<VulkanShaderModule> compileFromFile(const std::shared_ptr<VulkanDevice> &d,
                                                                const std::string &path) {
                std::ifstream file(path, std::ios::ate);
                if (!file.is_open())
                    throw std::runtime_error("Failed to read from file");

                const std::streamsize size = file.tellg();
                std::vector<char> buffer(size);
                file.seekg(0, std::ios_base::beg);
                file.read(buffer.data(), size);
                file.close();

                return compile(d, buffer);
            }
        };
    };
}
