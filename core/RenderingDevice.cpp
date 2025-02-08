#include "RenderingDevice.h"

#ifdef DEBUG_ENABLED
#include <GlslangToSpv.h>
#endif

#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <spdlog/spdlog.h>
#include <spirv_cross.hpp>
#include <disassemble.h>

#include "error/CantCreateError.h"
#include "error/Macros.h"

namespace Vixen {
    bool RenderingDevice::reflectShader(const std::vector<ShaderStageData> &stages, Shader *shader) {
        for (const auto &[stage, spirv]: stages) {
            const auto compiler = spirv_cross::Compiler(std::bit_cast<const uint32_t *>(spirv.data()),
                                                        spirv.size() / sizeof(uint32_t));
            auto resources = compiler.get_shader_resources();

            shader->stages.push_back(stage);

            if (!resources.push_constant_buffers.empty()) {
                const auto pushConstant = resources.push_constant_buffers[0];
                shader->pushConstantSize = compiler.get_active_buffer_ranges(pushConstant.id)[0].range;
                shader->pushConstantStages.push_back(stage);
            }

            for (const auto &uniformBuffer: resources.uniform_buffers) {
                shader->uniformSets.push_back({
                    .type = ShaderUniformType::UniformBuffer,
                    .binding = compiler.get_decoration(uniformBuffer.id, spv::DecorationBinding),
                    .length = static_cast<uint32_t>(compiler.get_declared_struct_size(
                        compiler.get_type(uniformBuffer.base_type_id)))
                });
            }

            for (const auto &sampler: resources.separate_samplers) {
                shader->uniformSets.push_back({
                    .type = ShaderUniformType::Sampler,
                    .binding = compiler.get_decoration(sampler.id, spv::DecorationBinding),
                    .length = 0
                });
            }

            for (const auto &sampledImage: resources.sampled_images) {
                shader->uniformSets.push_back({
                    .type = ShaderUniformType::CombinedImageSampler,
                    .binding = compiler.get_decoration(sampledImage.id, spv::DecorationBinding),
                    .length = 0
                });
            }
        }

        return true;
    }

    std::vector<std::byte> RenderingDevice::compileSpirvFromSource(ShaderStage stage, const std::string &source,
                                                                   ShaderLanguage language) {
        EShLanguage glslangLanguage;
        switch (stage) {
            case ShaderStage::Vertex:
                glslangLanguage = EShLangVertex;
                break;

            case ShaderStage::Fragment:
                glslangLanguage = EShLangFragment;
                break;

            case ShaderStage::TesselationControl:
                glslangLanguage = EShLangTessControl;
                break;

            case ShaderStage::TesselationEvaluation:
                glslangLanguage = EShLangTessEvaluation;
                break;

            case ShaderStage::Compute:
                glslangLanguage = EShLangCompute;
                break;

            case ShaderStage::Geometry:
                glslangLanguage = EShLangGeometry;
                break;

            default:
                ASSERT_THROW(false, CantCreateError, "Unknown shader stage");
        }

        glslang::InitializeProcess();

        glslang::TShader shader(glslangLanguage);
        auto src = source.data();
        shader.setStrings(&src, 1);
        shader.setEnvInput(glslang::EShSourceGlsl, glslangLanguage, glslang::EShClientVulkan, 160);
        shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_4);
        shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);

        glslang::TProgram program;

        // TODO: Add actual includer
        glslang::TShader::ForbidIncluder includer;

        auto messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
#ifdef DEBUG_ENABLED
        messages = static_cast<EShMessages>(messages | EShMsgDebugInfo);
#endif
        ASSERT_THROW(shader.parse(GetDefaultResources(), 160, false, messages), CantCreateError,
                     "Failed to parse SPIR-V");

        program.addShader(&shader);
        ASSERT_THROW(program.link(messages), CantCreateError, "Failed to link shader");

        glslang::SpvOptions options{
#ifdef DEBUG_ENABLED
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
        std::vector<uint32_t> binary{};
        GlslangToSpv(*program.getIntermediate(glslangLanguage), binary, &logger, &options);

#ifdef DEBUG_ENABLED
        std::stringstream stream;
        spv::Disassemble(stream, binary);
        spdlog::debug("Passed in GLSL source string:\n{}\n\nDisassembled SPIR-V:\n{}",
                      std::string_view(source.begin(), source.end()), stream.str());
#endif
        glslang::FinalizeProcess();

        std::vector<std::byte> result{binary.size() * sizeof(uint32_t)};
        memcpy(result.data(), binary.data(), binary.size() * sizeof(uint32_t));

        return result;
    }
}
