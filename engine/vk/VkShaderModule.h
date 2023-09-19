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

            static void InitResources(TBuiltInResource &Resources) {
                Resources.maxLights = 32;
                Resources.maxClipPlanes = 6;
                Resources.maxTextureUnits = 32;
                Resources.maxTextureCoords = 32;
                Resources.maxVertexAttribs = 64;
                Resources.maxVertexUniformComponents = 4096;
                Resources.maxVaryingFloats = 64;
                Resources.maxVertexTextureImageUnits = 32;
                Resources.maxCombinedTextureImageUnits = 80;
                Resources.maxTextureImageUnits = 32;
                Resources.maxFragmentUniformComponents = 4096;
                Resources.maxDrawBuffers = 32;
                Resources.maxVertexUniformVectors = 128;
                Resources.maxVaryingVectors = 8;
                Resources.maxFragmentUniformVectors = 16;
                Resources.maxVertexOutputVectors = 16;
                Resources.maxFragmentInputVectors = 15;
                Resources.minProgramTexelOffset = -8;
                Resources.maxProgramTexelOffset = 7;
                Resources.maxClipDistances = 8;
                Resources.maxComputeWorkGroupCountX = 65535;
                Resources.maxComputeWorkGroupCountY = 65535;
                Resources.maxComputeWorkGroupCountZ = 65535;
                Resources.maxComputeWorkGroupSizeX = 1024;
                Resources.maxComputeWorkGroupSizeY = 1024;
                Resources.maxComputeWorkGroupSizeZ = 64;
                Resources.maxComputeUniformComponents = 1024;
                Resources.maxComputeTextureImageUnits = 16;
                Resources.maxComputeImageUniforms = 8;
                Resources.maxComputeAtomicCounters = 8;
                Resources.maxComputeAtomicCounterBuffers = 1;
                Resources.maxVaryingComponents = 60;
                Resources.maxVertexOutputComponents = 64;
                Resources.maxGeometryInputComponents = 64;
                Resources.maxGeometryOutputComponents = 128;
                Resources.maxFragmentInputComponents = 128;
                Resources.maxImageUnits = 8;
                Resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
                Resources.maxCombinedShaderOutputResources = 8;
                Resources.maxImageSamples = 0;
                Resources.maxVertexImageUniforms = 0;
                Resources.maxTessControlImageUniforms = 0;
                Resources.maxTessEvaluationImageUniforms = 0;
                Resources.maxGeometryImageUniforms = 0;
                Resources.maxFragmentImageUniforms = 8;
                Resources.maxCombinedImageUniforms = 8;
                Resources.maxGeometryTextureImageUnits = 16;
                Resources.maxGeometryOutputVertices = 256;
                Resources.maxGeometryTotalOutputComponents = 1024;
                Resources.maxGeometryUniformComponents = 1024;
                Resources.maxGeometryVaryingComponents = 64;
                Resources.maxTessControlInputComponents = 128;
                Resources.maxTessControlOutputComponents = 128;
                Resources.maxTessControlTextureImageUnits = 16;
                Resources.maxTessControlUniformComponents = 1024;
                Resources.maxTessControlTotalOutputComponents = 4096;
                Resources.maxTessEvaluationInputComponents = 128;
                Resources.maxTessEvaluationOutputComponents = 128;
                Resources.maxTessEvaluationTextureImageUnits = 16;
                Resources.maxTessEvaluationUniformComponents = 1024;
                Resources.maxTessPatchComponents = 120;
                Resources.maxPatchVertices = 32;
                Resources.maxTessGenLevel = 64;
                Resources.maxViewports = 16;
                Resources.maxVertexAtomicCounters = 0;
                Resources.maxTessControlAtomicCounters = 0;
                Resources.maxTessEvaluationAtomicCounters = 0;
                Resources.maxGeometryAtomicCounters = 0;
                Resources.maxFragmentAtomicCounters = 8;
                Resources.maxCombinedAtomicCounters = 8;
                Resources.maxAtomicCounterBindings = 1;
                Resources.maxVertexAtomicCounterBuffers = 0;
                Resources.maxTessControlAtomicCounterBuffers = 0;
                Resources.maxTessEvaluationAtomicCounterBuffers = 0;
                Resources.maxGeometryAtomicCounterBuffers = 0;
                Resources.maxFragmentAtomicCounterBuffers = 1;
                Resources.maxCombinedAtomicCounterBuffers = 1;
                Resources.maxAtomicCounterBufferSize = 16384;
                Resources.maxTransformFeedbackBuffers = 4;
                Resources.maxTransformFeedbackInterleavedComponents = 64;
                Resources.maxCullDistances = 8;
                Resources.maxCombinedClipAndCullDistances = 8;
                Resources.maxSamples = 4;
                Resources.maxMeshOutputVerticesNV = 256;
                Resources.maxMeshOutputPrimitivesNV = 512;
                Resources.maxMeshWorkGroupSizeX_NV = 32;
                Resources.maxMeshWorkGroupSizeY_NV = 1;
                Resources.maxMeshWorkGroupSizeZ_NV = 1;
                Resources.maxTaskWorkGroupSizeX_NV = 32;
                Resources.maxTaskWorkGroupSizeY_NV = 1;
                Resources.maxTaskWorkGroupSizeZ_NV = 1;
                Resources.maxMeshViewCountNV = 4;
                Resources.limits.nonInductiveForLoops = 1;
                Resources.limits.whileLoops = 1;
                Resources.limits.doWhileLoops = 1;
                Resources.limits.generalUniformIndexing = 1;
                Resources.limits.generalAttributeMatrixVectorIndexing = 1;
                Resources.limits.generalVaryingIndexing = 1;
                Resources.limits.generalSamplerIndexing = 1;
                Resources.limits.generalVariableIndexing = 1;
                Resources.limits.generalConstantMatrixVectorIndexing = 1;
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

                TBuiltInResource resources{};
                InitResources(resources);

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

                if (!shader.parse(&resources, 100, false, messages)) {
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
