#pragma once

#ifdef DEBUG

#include <glslang/SPIRV/disassemble.h>

#endif

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>
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
                const std::vector<IO> &inputs,
                const std::vector<IO> &outputs,
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

            std::vector<IO> inputs{};

            std::vector<IO> outputs{};

            uint32_t getTypeSize(const glslang::TType *type) {
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

            Builder &addInput(const IO &input) {
                inputs.push_back(input);
                return *this;
            }

            Builder &addOutput(const IO &output) {
                outputs.push_back(output);
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

                shader.setEntryPoint(entrypoint.c_str());
                shader.setSourceEntryPoint(entrypoint.c_str());

                auto messages = (EShMessages) (EShMsgSpvRules | EShMsgVulkanRules);
                glslang::TShader::ForbidIncluder includer;

                if (!shader.parse(GetDefaultResources(), VIXEN_VK_SPIRV_VERSION, false, messages)) {
                    spdlog::error("Failed to parse shader; {}", shader.getInfoLog());
                    throw std::runtime_error("Failed to parse shader");
                }

                program.addShader(&shader);
                if (!program.link(messages)) {
                    spdlog::error("Failed to link shader program; {}", shader.getInfoLog());
                    throw std::runtime_error("Failed to link shader program");
                }

                glslang::SpvOptions options;
#ifdef DEBUG
                options.generateDebugInfo = true;
                options.disableOptimizer = true;
                options.optimizeSize = false;
                options.stripDebugInfo = false;
#else
                options.disableOptimizer = false;
                options.disableOptimizer = false;
                options.optimizeSize = true;
                options.stripDebugInfo = true;
#endif
                options.validate = true;
                spv::SpvBuildLogger logger;
                glslang::GlslangToSpv(*program.getIntermediate(s), binary, &logger, &options);

#ifdef DEBUG
                std::stringstream stream;
                spv::Disassemble(stream, binary);
                spdlog::debug("Passed in GLSL source string:\n{}\n\nDisassembled SPIR-V:\n{}",
                              std::string_view(source.begin(), source.end()), stream.str());
#endif

                if (!program.buildReflection()) {
                    spdlog::error("Failed to process shader reflection; {}", shader.getInfoLog());
                    throw std::runtime_error("Failed to link shader program");
                }

                for (auto i = 0; i < program.getNumPipeInputs(); i++) {
                    const auto &input = program.getPipeInput(i);
                    auto type = input.getType();

                    inputs.push_back(
                            {
                                    .name = input.name,
                                    .size = getTypeSize(type),
                                    .location = input.index == -1 ?
                                                std::nullopt :
                                                std::optional{static_cast<uint32_t>(input.index)},
                                    .binding = input.counterIndex == -1 ?
                                               std::nullopt :
                                               std::optional{static_cast<uint32_t>(input.counterIndex)}
                            }
                    );
                }

                for (auto i = 0; i < program.getNumPipeOutputs(); i++) {
                    const auto &output = program.getPipeOutput(i);

                    outputs.push_back(
                            {
                                    .name = output.name,
                                    .size = static_cast<uint32_t>(output.size),
                                    .location = output.index == -1 ?
                                                std::nullopt :
                                                std::optional{static_cast<uint32_t>(output.index)},
                                    .binding = output.counterIndex == -1 ?
                                               std::nullopt :
                                               std::optional{static_cast<uint32_t>(output.counterIndex)}
                            }
                    );
                }

                if (!logger.getAllMessages().empty())
                    spdlog::warn("Error messages were generated during shader module compilation;\n{}",
                                 logger.getAllMessages());
                spdlog::trace("Compiled shader to SPIR-V binary {}", spdlog::to_hex(binary.begin(), binary.end()));

                glslang::FinalizeProcess();

                return std::make_shared<VkShaderModule>(d, stage, binary, inputs, outputs, entrypoint);
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
