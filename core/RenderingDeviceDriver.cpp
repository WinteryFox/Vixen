#include "RenderingDeviceDriver.h"

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
#include "shader/Shader.h"
#include "shader/ShaderUniform.h"
#include "shader/ShaderUniformType.h"

namespace Vixen {
    bool RenderingDeviceDriver::reflectShader(
        const std::vector<ShaderStageData>& stages,
        Shader* shader
    ) {
        for (const auto& [stage, spirv] : stages) {
            const auto compiler = spirv_cross::Compiler(
                std::bit_cast<const uint32_t*>(spirv.data()),
                spirv.size() / sizeof(uint32_t)
            );
            auto resources = compiler.get_shader_resources();

            auto getDescriptorCount = [&](const spirv_cross::Resource& resource) -> std::optional<uint32_t> {
                const auto& type = compiler.get_type(resource.type_id);

                uint32_t count = 1;

                for (size_t i = 0; i < type.array.size(); ++i) {
                    if (!type.array_size_literal[i])
                        return std::nullopt;

                    count *= type.array[i];
                }

                return count;
            };

            auto addUniform = [&](
                const spirv_cross::Resource& resource,
                const ShaderUniformType type,
                const uint32_t length = 0
            ) -> bool {
                const auto count = getDescriptorCount(resource);

                if (!count)
                    return false;

                const ShaderUniform uniform{
                    .type = type,
                    .set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet),
                    .binding = compiler.get_decoration(resource.id, spv::DecorationBinding),
                    .count = *count,
                    .length = length,
                    .stages = stage
                };

                const auto existing = std::ranges::find_if(
                    shader->uniformSets,
                    [&](const ShaderUniform& other) {
                        return other.set == uniform.set && other.binding == uniform.binding;
                    });

                if (existing == shader->uniformSets.end()) {
                    shader->uniformSets.push_back(uniform);

                    return true;
                }

                if (existing->type != uniform.type || existing->count != uniform.count || existing->length != uniform.
                    length)
                    return false;

                existing->stages |= stage;

                return true;
            };

            if (!resources.push_constant_buffers.empty()) {
                const auto& pushConstant = resources.push_constant_buffers.front();

                const auto& type = compiler.get_type(pushConstant.base_type_id);

                const auto size = static_cast<uint32_t>(compiler.get_declared_struct_size(type));

                shader->pushConstantSize = std::max(shader->pushConstantSize, size);

                shader->pushConstantStages |= stage;
            }

            for (const auto& resource : resources.separate_samplers)
                if (!addUniform(resource, ShaderUniformType::Sampler))
                    return false;

            for (const auto& resource : resources.separate_images)
                if (!addUniform(resource, ShaderUniformType::SampledImage))
                    return false;

            for (const auto& resource : resources.sampled_images)
                if (!addUniform(resource, ShaderUniformType::CombinedImageSampler))
                    return false;

            for (const auto& resource : resources.storage_images)
                if (!addUniform(resource, ShaderUniformType::StorageImage))
                    return false;

            for (const auto& resource : resources.uniform_buffers) {
                const auto& type = compiler.get_type(resource.base_type_id);
                const auto size = static_cast<uint32_t>(compiler.get_declared_struct_size(type));

                if (!addUniform(resource, ShaderUniformType::UniformBuffer, size))
                    return false;
            }

            for (const auto& resource : resources.storage_buffers) {
                const auto& type = compiler.get_type(resource.base_type_id);
                const auto size = static_cast<uint32_t>(compiler.get_declared_struct_size(type));

                if (!addUniform(resource, ShaderUniformType::StorageBuffer, size))
                    return false;
            }

            for (const auto& resource : resources.subpass_inputs)
                if (!addUniform(resource, ShaderUniformType::InputAttachment))
                    return false;

            shader->stages |= stage;
        }

        return true;
    }

    std::vector<std::byte> RenderingDeviceDriver::compileSpirvFromSource(
        ShaderStageBits stage,
        const std::string& source,
        ShaderLanguage language
    ) {
        EShLanguage glslangLanguage;
        switch (stage) {
            case ShaderStageBits::Vertex:
                glslangLanguage = EShLangVertex;
                break;

            case ShaderStageBits::Fragment:
                glslangLanguage = EShLangFragment;
                break;

            case ShaderStageBits::TesselationControl:
                glslangLanguage = EShLangTessControl;
                break;

            case ShaderStageBits::TesselationEvaluation:
                glslangLanguage = EShLangTessEvaluation;
                break;

            case ShaderStageBits::Compute:
                glslangLanguage = EShLangCompute;
                break;

            case ShaderStageBits::Geometry:
                glslangLanguage = EShLangGeometry;
                break;

            default:
                std::unreachable();
        }

        glslang::InitializeProcess();

        glslang::TShader shader(glslangLanguage);
        auto src = source.data();
        shader.setStrings(&src, 1);
        shader.setEnvInput(glslang::EShSourceGlsl, glslangLanguage, glslang::EShClientVulkan, 160);
        shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
        shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);

        glslang::TProgram program;

        // TODO: Add actual includer
        glslang::TShader::ForbidIncluder includer;

        auto messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
        #ifdef DEBUG_ENABLED
        messages = static_cast<EShMessages>(messages | EShMsgDebugInfo);
        #endif

        if (!shader.parse(GetDefaultResources(), 160, false, messages))
            error<CantCreateError>("Failed to parse SPIR-V");

        program.addShader(&shader);
        if (!program.link(messages))
            error<CantCreateError>("Failed to link shader");

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
        spdlog::debug(
            "Passed in GLSL source string:\n{}\n\nDisassembled SPIR-V:\n{}",
            std::string_view(source.begin(), source.end()),
            stream.str()
        );
        #endif
        glslang::FinalizeProcess();

        std::vector<std::byte> result{binary.size() * sizeof(uint32_t)};
        memcpy(result.data(), binary.data(), binary.size() * sizeof(uint32_t));

        return result;
    }
}
