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

namespace Vixen::Engine {
    class VkShaderModule : public ShaderModule {
        ::VkShaderModule module = VK_NULL_HANDLE;

        Stage stage;

        std::shared_ptr<Device> device;

    public:
        VkShaderModule(const std::shared_ptr<Device> &device, Stage stage, const std::vector<uint32_t> &binary,
                       const std::string &entrypoint = "main");

        VkShaderModule(const VkShaderModule &) = delete;

        VkShaderModule &operator=(const VkShaderModule &) = delete;

        ~VkShaderModule();

        [[nodiscard]] ::VkShaderModule getModule() const;

        class Builder {
        private:
            Stage stage;

            std::vector<uint32_t> binary;

            std::string entrypoint = "main";

        public:
            Builder &setStage(Stage s) {
                stage = s;
                return *this;
            }

            Builder &setBinary(const std::vector<uint32_t> &data) {
                binary = data;
                return *this;
            }

            Builder &compile(const std::vector<char> &source) {
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
                shader.setEnvInput(glslang::EShSourceGlsl, s, glslang::EShClientVulkan, 100);
                shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
                shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);
#ifdef DEBUG
                shader.setDebugInfo(true);
#endif

                shader.setEntryPoint(entrypoint.c_str());
                shader.setSourceEntryPoint(entrypoint.c_str());

                auto messages = (EShMessages) (EShMsgSpvRules | EShMsgVulkanRules);
                glslang::TShader::ForbidIncluder includer;

                if (!shader.parse(GetDefaultResources(), 100, false, messages)) {
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
                spdlog::debug("Disassembled SPIR-V:\n{}", stream.str());
#endif

                if (!program.buildReflection()) {
                    spdlog::error("Failed to process shader reflection; {}", shader.getInfoLog());
                    throw std::runtime_error("Failed to link shader program");
                }

                if (!logger.getAllMessages().empty())
                    spdlog::warn("Error messages were generated during shader module compilation;\n{}",
                                 logger.getAllMessages());
                spdlog::trace("Compiled shader to SPIR-V binary {}", spdlog::to_hex(binary.begin(), binary.end()));

                glslang::FinalizeProcess();

                return *this;
            }

            Builder &compile(const std::string &source) {
                return compile(std::vector(source.begin(), source.end()));
            }

            Builder &compileFromFile(const std::string &path) {
                std::ifstream file(path, std::ios::ate);
                if (!file.is_open())
                    throw std::runtime_error("Failed to read from file");

                std::streamsize size = file.tellg();
                std::vector<char> buffer(size);
                file.seekg(0);
                file.read(buffer.data(), size);
                file.close();

                return compile(buffer);
            }

            Builder &setEntrypoint(const std::string &entry) {
                entrypoint = entry;
                return *this;
            }

            std::shared_ptr<VkShaderModule> build(const std::shared_ptr<Device> &d) {
                return std::make_shared<VkShaderModule>(d, stage, binary, entrypoint);
            }
        };
    };
}
