#pragma once

#ifdef DEBUG

#include <glslang/SPIRV/disassemble.h>

#endif

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <spirv_reflect.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <fstream>
#include <sstream>
#include "../ShaderModule.h"
#include "Vulkan.h"
#include "Device.h"

#define VIXEN_VK_SPIRV_VERSION 130

namespace Vixen::Vk {
    class VkShaderModule final : public ShaderModule {
        ::VkShaderModule module;

        std::shared_ptr<Device> device;

    public:
        VkShaderModule(
            const std::shared_ptr<Device>& device,
            Stage stage,
            const std::vector<uint32_t>& binary,
            const std::vector<Binding>& bindings,
            const std::vector<IO>& inputs,
            const std::vector<Uniform>& uniformBuffers,
            const std::string& entrypoint = "main"
        );

        VkShaderModule(const VkShaderModule&) = delete;

        VkShaderModule& operator=(const VkShaderModule&) = delete;

        ~VkShaderModule() override;

        [[nodiscard]] VkPipelineShaderStageCreateInfo createInfo() const;

        [[nodiscard]] std::vector<VkDescriptorSetLayoutBinding> createBindings() const;

        [[nodiscard]] std::shared_ptr<Device> getDevice() const;

        class Builder {
            // TODO: This builder has slightly confusing API, you can compile before setting the stage meaning its possible to mistakenly have the wrong stage set at compile time
            Stage stage;

            std::vector<uint32_t> binary{};

            std::string entrypoint = "main";

            std::vector<Binding> bindings{};

            std::vector<IO> inputs{};

            std::vector<Uniform> uniforms{};

        public:
            explicit Builder(const Stage stage) : stage(stage) {}

            Builder& setStage(const Stage s) {
                stage = s;
                return *this;
            }

            Builder& setBinary(const std::vector<uint32_t>& data) {
                binary = data;
                return *this;
            }

            Builder& setEntrypoint(const std::string& entry) {
                entrypoint = entry;
                return *this;
            }

            Builder& addBinding(const Binding& binding) {
                bindings.push_back(binding);
                return *this;
            }

            Builder& addInput(const IO& input) {
                inputs.push_back(input);
                return *this;
            }

            std::shared_ptr<VkShaderModule> compile(const std::shared_ptr<Device>& d, const std::vector<char>& source) {
                EShLanguage s;
                switch (stage) {
                case Stage::Vertex:
                    s = EShLangVertex;
                    break;
                case Stage::Fragment:
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
                    error("Failed to parse shader; {}", shader.getInfoLog());

                program.addShader(&shader);
                if (!program.link(messages))
                    error("Failed to link shader program; {}", shader.getInfoLog());

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

                auto resources = c.get_shader_resources();
                for (const auto& [id, type_id, base_type_id, name] : resources.uniform_buffers) {
                    uint32_t binding = c.get_decoration(id, spv::DecorationBinding);

                    uniforms.push_back({
                        .stage = stage,
                        .binding = binding,
                        .type = Uniform::Type::Buffer
                    });
                }

                for (const auto& [id, type_id, base_type_id, name] : resources.sampled_images) {
                    uint32_t binding = c.get_decoration(id, spv::DecorationBinding);

                    uniforms.push_back({
                        .stage = stage,
                        .binding = binding,
                        .type = Uniform::Type::Sampler
                    });
                }

                if (!logger.getAllMessages().empty())
                    spdlog::warn("Error messages were generated during shader module compilation;\n{}",
                                 logger.getAllMessages());
                spdlog::trace("Compiled shader to SPIR-V binary {}", spdlog::to_hex(binary.begin(), binary.end()));

                glslang::FinalizeProcess();

                return std::make_shared<VkShaderModule>(d, stage, binary, bindings, inputs, uniforms, entrypoint);
            }

            std::shared_ptr<VkShaderModule> compile(const std::shared_ptr<Device>& d, const std::string& source) {
                return compile(d, std::vector(source.begin(), source.end()));
            }

            std::shared_ptr<VkShaderModule> compileFromFile(const std::shared_ptr<Device>& d, const std::string& path) {
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
