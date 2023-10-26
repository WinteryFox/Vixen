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
#include "../ShaderModule.h"
#include "Vulkan.h"
#include "Device.h"

#define VIXEN_VK_SPIRV_VERSION 130

namespace Vixen::Vk {
    class VkShaderModule : public ShaderModule {
        ::VkShaderModule module;

        std::shared_ptr<Device> device;

    public:
        VkShaderModule(
                const std::shared_ptr<Device> &device,
                Stage stage,
                const std::vector<uint32_t> &binary,
                const std::vector<Binding> &bindings,
                const std::vector<IO> &inputs,
                const std::string &entrypoint = "main"
        );

        VkShaderModule(const VkShaderModule &) = delete;

        VkShaderModule &operator=(const VkShaderModule &) = delete;

        ~VkShaderModule();

        [[nodiscard]] ::VkShaderModule getModule() const;

        class Builder {
            // TODO: This builder has slightly confusing API, you can compile before setting the stage meaning its possible to mistakenly have the wrong stage set at compile time
        private:
            Stage stage;

            std::vector<uint32_t> binary;

            std::string entrypoint = "main";

            std::vector<Binding> bindings{};

            std::vector<IO> inputs{};

            static uint32_t getTypeSize(const glslang::TType *type) {
                uint32_t size;

                switch (type->getBasicType()) {
                    case glslang::EbtVoid:
                        size = 0;
                        break;
                    case glslang::EbtFloat:
                        size = sizeof(float);
                        break;
                    case glslang::EbtDouble:
                        size = sizeof(double);
                        break;
                    case glslang::EbtFloat16:
                        size = sizeof(long);
                        break;
                    case glslang::EbtInt8:
                        size = sizeof(int8_t);
                        break;
                    case glslang::EbtUint8:
                        size = sizeof(uint8_t);
                        break;
                    case glslang::EbtInt16:
                        size = sizeof(int16_t);
                        break;
                    case glslang::EbtUint16:
                        size = sizeof(uint16_t);
                        break;
                    case glslang::EbtInt:
                        size = sizeof(int32_t);
                        break;
                    case glslang::EbtUint:
                        size = sizeof(uint32_t);
                        break;
                    case glslang::EbtInt64:
                        size = sizeof(int64_t);
                        break;
                    case glslang::EbtUint64:
                        size = sizeof(uint64_t);
                        break;
                    case glslang::EbtBool:
                        size = sizeof(bool);
                        break;
                    case glslang::EbtAtomicUint:
                        // TODO
                        size = 0;
                        break;
                    case glslang::EbtSampler:
                        size = 0;
                        break;
                    case glslang::EbtStruct:
                        size = 0;
                        break;
                    case glslang::EbtBlock:
                        size = 0;
                        break;
                    case glslang::EbtAccStruct:
                        size = 0;
                        break;
                    case glslang::EbtReference:
                        size = 0;
                        break;
                    case glslang::EbtRayQuery:
                        size = 0;
                        break;
                    case glslang::EbtHitObjectNV:
                        size = 0;
                        break;
                    case glslang::EbtCoopmat:
                        size = 0;
                        break;
                    case glslang::EbtSpirvType:
                        size = 0;
                        break;
                    case glslang::EbtString:
                        size = 0;
                        break;
                    case glslang::EbtNumTypes:
                        size = 0;
                        break;
                }

                if (type->isVector())
                    size *= type->getVectorSize();

                return size;
            }

        public:
            Builder &setStage(Stage s) {
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

            Builder &addBinding(Binding binding) {
                bindings.push_back(binding);
                return *this;
            }

            Builder &addInput(const IO &input) {
                inputs.push_back(input);
                return *this;
            }

            std::shared_ptr<VkShaderModule> compile(const std::shared_ptr<Device> &d, const std::vector<char> &source) {
                EShLanguage s;
                switch (stage) {
                    case Stage::VERTEX:
                        s = EShLanguage::EShLangVertex;
                        break;
                    case Stage::FRAGMENT:
                        s = EShLanguage::EShLangFragment;
                        break;
                    default:
                        spdlog::error("Unsupported stage for shader module");
                        throw std::runtime_error("Unsupported stage for shader module");
                }

                glslang::InitializeProcess();
                glslang::TShader shader{s};
                glslang::TProgram program;

                auto src = source.data();
                shader.setStrings(&src, 1);
                shader.setEnvInput(glslang::EShSourceGlsl, s, glslang::EShClientVulkan, VIXEN_VK_SPIRV_VERSION);
                shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
                shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);
#ifdef DEBUG
                shader.setDebugInfo(true);
#endif
                shader.setAutoMapLocations(true);

                shader.setEntryPoint(entrypoint.c_str());
                shader.setSourceEntryPoint(entrypoint.c_str());

                auto messages = (EShMessages) (EShMsgSpvRules | EShMsgVulkanRules);
                glslang::TShader::ForbidIncluder includer;

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
                glslang::GlslangToSpv(*program.getIntermediate(s), binary, &logger, &options);

#ifdef DEBUG
                std::stringstream stream;
                spv::Disassemble(stream, binary);
                spdlog::debug("Passed in GLSL source string:\n{}\n\nDisassembled SPIR-V:\n{}",
                              std::string_view(source.begin(), source.end()), stream.str());
#endif

                spirv_cross::CompilerReflection c{binary};
                auto resources = c.get_shader_resources();

                for (const auto &uniformBuffer: resources.uniform_buffers) {
                    /*const auto &binding = c.get_decoration(input.id, spv::DecorationBinding);
                    const auto &location = c.get_decoration(input.id, spv::DecorationLocation);

                    inputs.push_back(
                            {
                                    .name = input.name,
                                    .size = 3 * sizeof(float),
                                    .binding = binding == 0 ?
                                               std::nullopt :
                                               std::optional{static_cast<uint32_t>(binding)},
                                    .location = location == 0 ?
                                                std::nullopt :
                                                std::optional{static_cast<uint32_t>(location)},
                            }
                    );*/
                }

                if (!logger.getAllMessages().empty())
                    spdlog::warn("Error messages were generated during shader module compilation;\n{}",
                                 logger.getAllMessages());
                spdlog::trace("Compiled shader to SPIR-V binary {}", spdlog::to_hex(binary.begin(), binary.end()));

                glslang::FinalizeProcess();

                return std::make_shared<VkShaderModule>(d, stage, binary, bindings, inputs, entrypoint);
            }

            std::shared_ptr<VkShaderModule> compile(const std::shared_ptr<Device> &d, const std::string &source) {
                return compile(d, std::vector(source.begin(), source.end()));
            }

            std::shared_ptr<VkShaderModule> compileFromFile(const std::shared_ptr<Device> &d, const std::string &path) {
                std::ifstream file(path, std::ios::ate);
                if (!file.is_open())
                    throw std::runtime_error("Failed to read from file");

                std::streamsize size = file.tellg();
                std::vector<char> buffer(size);
                file.seekg(0);
                file.read(buffer.data(), size);
                file.close();

                return compile(d, buffer);
            }
        };
    };
}
