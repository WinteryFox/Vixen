#include "RenderingDeviceDriver.h"

#include <cstring>
#include <format>
#include <limits>

#ifdef DEBUG_ENABLED
#include <GlslangToSpv.h>
#endif

#include <disassemble.h>
#include <spirv_cross.hpp>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <spdlog/spdlog.h>

#include "error/CantCreateError.h"
#include "error/Macros.h"
#include "error/Shader.h"
#include "shader/Shader.h"
#include "shader/ShaderUniform.h"
#include "shader/ShaderUniformType.h"

namespace Vixen {
    auto RenderingDeviceDriver::reflectShader(
        const std::vector<ShaderStageData>& stages,
        Shader* shader
    ) -> std::expected<void, ShaderReflectionError> {
        auto fail = [](
            const ShaderReflectionErrorCode type,
            std::string detail,
            const std::optional<ShaderStageBits> stage = std::nullopt,
            std::string resourceName = {},
            const std::optional<uint32_t> set = std::nullopt,
            const std::optional<uint32_t> binding = std::nullopt,
            const std::optional<uint64_t> expected = std::nullopt,
            const std::optional<uint64_t> actual = std::nullopt
        ) -> std::expected<void, ShaderReflectionError> {
            return std::unexpected(
                ShaderReflectionError{
                    .type = type,
                    .stages = stage,
                    .resourceName = std::move(resourceName),
                    .set = set,
                    .binding = binding,
                    .expected = expected,
                    .actual = actual,
                    .detail = std::move(detail)
                }
            );
        };

        if (shader == nullptr)
            return fail(ShaderReflectionErrorCode::NullOutputShader, "Output shader is null");

        if (stages.empty())
            return fail(ShaderReflectionErrorCode::NoShaderStages, "Shader contains no stages");

        ShaderStageFlags suppliedStages{};
        bool hasCompute = false;
        bool hasGraphics = false;

        for (const auto& stageData : stages) {
            if (suppliedStages.contains(stageData.stage))
                return fail(
                    ShaderReflectionErrorCode::DuplicateShaderStage,
                    "The same shader stage was supplied more than once",
                    stageData.stage
                );

            suppliedStages |= stageData.stage;
            hasCompute |= stageData.stage == ShaderStageBits::Compute;
            hasGraphics |= stageData.stage != ShaderStageBits::Compute;
        }

        if (hasCompute && hasGraphics)
            return fail(
                ShaderReflectionErrorCode::IncompatibleShaderStages,
                "Compute and graphics stages cannot be combined in one shader"
            );

        for (const auto& [stage, spirv] : stages) {
            if (spirv.empty())
                return fail(ShaderReflectionErrorCode::EmptySpirv, "SPIR-V module is empty", stage);

            if (spirv.size() % sizeof(uint32_t) != 0)
                return fail(
                    ShaderReflectionErrorCode::InvalidSpirvSize,
                    "SPIR-V byte count is not a multiple of four",
                    stage,
                    {},
                    std::nullopt,
                    std::nullopt,
                    std::nullopt,
                    spirv.size()
                );

            std::vector<uint32_t> words(spirv.size() / sizeof(uint32_t));
            std::memcpy(words.data(), spirv.data(), spirv.size());

            constexpr uint32_t spirvMagic = 0x07230203u;
            if (words.front() != spirvMagic)
                return fail(
                    ShaderReflectionErrorCode::InvalidSpirvMagic,
                    "SPIR-V module has an invalid magic number",
                    stage,
                    {},
                    std::nullopt,
                    std::nullopt,
                    spirvMagic,
                    words.front()
                );

            try {
                const auto compiler = spirv_cross::Compiler(words);
                const auto resources = compiler.get_shader_resources();

                const auto entryPoints = compiler.get_entry_points_and_stages();
                if (entryPoints.empty())
                    return fail(ShaderReflectionErrorCode::NoEntryPoint, "SPIR-V module has no entry point", stage);

                if (entryPoints.size() > 1)
                    return fail(
                        ShaderReflectionErrorCode::MultipleEntryPoints,
                        "SPIR-V module has more than one entry point",
                        stage,
                        {},
                        std::nullopt,
                        std::nullopt,
                        1,
                        entryPoints.size()
                    );

                if (entryPoints.front().name != "main")
                    return fail(
                        ShaderReflectionErrorCode::InvalidEntryPointName,
                        std::format("SPIR-V entry point is '{}', expected 'main'", entryPoints.front().name),
                        stage
                    );

                auto getDescriptorCount = [&](
                    const spirv_cross::Resource& resource
                ) -> std::expected<uint32_t, ShaderReflectionError> {
                const auto& type = compiler.get_type(resource.type_id);

                uint64_t count = 1;

                for (size_t i = 0; i < type.array.size(); ++i) {
                    if (!type.array_size_literal[i])
                        return std::unexpected(
                            ShaderReflectionError{
                                .type = ShaderReflectionErrorCode::RuntimeDescriptorArrayUnsupported,
                                .stages = stage,
                                .resourceName = resource.name,
                                .set = std::nullopt,
                                .binding = std::nullopt,
                                .expected = std::nullopt,
                                .actual = std::nullopt,
                                .detail = "Runtime-sized descriptor arrays are not supported"
                            }
                        );

                    const uint64_t dimension = type.array[i];
                    if (dimension == 0)
                        return std::unexpected(
                            ShaderReflectionError{
                                .type = ShaderReflectionErrorCode::ZeroDescriptorCount,
                                .stages = stage,
                                .resourceName = resource.name,
                                .set = std::nullopt,
                                .binding = std::nullopt,
                                .expected = std::nullopt,
                                .actual = 0,
                                .detail = "Descriptor array has a zero-length dimension"
                            }
                        );

                    if (count > std::numeric_limits<uint32_t>::max() / dimension)
                        return std::unexpected(
                            ShaderReflectionError{
                                .type = ShaderReflectionErrorCode::DescriptorCountOverflow,
                                .stages = stage,
                                .resourceName = resource.name,
                                .set = std::nullopt,
                                .binding = std::nullopt,
                                .expected = std::numeric_limits<uint32_t>::max(),
                                .actual = count * dimension,
                                .detail = "Descriptor array count overflows uint32_t"
                            }
                        );

                    count *= dimension;
                }

                return static_cast<uint32_t>(count);
                };

                auto addUniform = [&] (
                    const spirv_cross::Resource& resource,
                    const ShaderUniformType type,
                    const uint32_t length = 0
                ) -> std::expected<void, ShaderReflectionError> {
                    if (!compiler.has_decoration(resource.id, spv::DecorationDescriptorSet))
                        return std::unexpected(
                            ShaderReflectionError{
                                .type = ShaderReflectionErrorCode::MissingDescriptorSet,
                                .stages = stage,
                                .resourceName = resource.name,
                                .set = std::nullopt,
                                .binding = compiler.has_decoration(resource.id, spv::DecorationBinding)
                                    ? std::optional(compiler.get_decoration(resource.id, spv::DecorationBinding))
                                    : std::nullopt,
                                .expected = std::nullopt,
                                .actual = std::nullopt,
                                .detail = "Descriptor resource has no set decoration"
                            }
                        );

                    if (!compiler.has_decoration(resource.id, spv::DecorationBinding))
                        return std::unexpected(
                            ShaderReflectionError{
                                .type = ShaderReflectionErrorCode::MissingDescriptorBinding,
                                .stages = stage,
                                .resourceName = resource.name,
                                .set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet),
                                .binding = std::nullopt,
                                .expected = std::nullopt,
                                .actual = std::nullopt,
                                .detail = "Descriptor resource has no binding decoration"
                            }
                        );

                    const auto count = getDescriptorCount(resource);
                    if (!count)
                        return std::unexpected(count.error());

                    const ShaderUniform uniform{
                        .type = type,
                        .set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet),
                        .binding = compiler.get_decoration(resource.id, spv::DecorationBinding),
                        .count = count.value(),
                        .length = length,
                        .stages = stage
                    };

                    const auto existing = std::ranges::find_if(
                        shader->uniformSets,
                        [&](const ShaderUniform& other) {
                            return other.set == uniform.set && other.binding == uniform.binding;
                        }
                    );

                    if (existing == shader->uniformSets.end()) {
                        shader->uniformSets.push_back(uniform);
                        return {};
                    }

                    if (existing->type != uniform.type)
                        return std::unexpected(
                            ShaderReflectionError{
                                .type = ShaderReflectionErrorCode::DescriptorTypeConflict,
                                .stages = stage,
                                .resourceName = resource.name,
                                .set = uniform.set,
                                .binding = uniform.binding,
                                .expected = static_cast<uint64_t>(existing->type),
                                .actual = static_cast<uint64_t>(uniform.type),
                                .detail = "Descriptor type conflicts with another shader stage"
                            }
                        );

                    if (existing->count != uniform.count)
                        return std::unexpected(
                            ShaderReflectionError{
                                .type = ShaderReflectionErrorCode::DescriptorCountConflict,
                                .stages = stage,
                                .resourceName = resource.name,
                                .set = uniform.set,
                                .binding = uniform.binding,
                                .expected = existing->count,
                                .actual = uniform.count,
                                .detail = "Descriptor count conflicts with another shader stage"
                            }
                        );

                    if (existing->length != uniform.length)
                        return std::unexpected(
                            ShaderReflectionError{
                                .type = ShaderReflectionErrorCode::DescriptorSizeConflict,
                                .stages = stage,
                                .resourceName = resource.name,
                                .set = uniform.set,
                                .binding = uniform.binding,
                                .expected = existing->length,
                                .actual = uniform.length,
                                .detail = "Descriptor buffer size conflicts with another shader stage"
                            }
                        );

                    existing->stages |= stage;
                    return {};
                };

                if (resources.push_constant_buffers.size() > 1)
                    return fail(
                        ShaderReflectionErrorCode::MultiplePushConstantBlocks,
                        "Shader stage declares more than one push-constant block",
                        stage,
                        resources.push_constant_buffers[1].name,
                        std::nullopt,
                        std::nullopt,
                        1,
                        resources.push_constant_buffers.size()
                    );

                if (!resources.push_constant_buffers.empty()) {
                    const auto& pushConstant = resources.push_constant_buffers.front();

                    const auto& type = compiler.get_type(pushConstant.base_type_id);
                    const uint64_t rawSize = compiler.get_declared_struct_size(type);

                    if (rawSize == 0)
                        return fail(
                            ShaderReflectionErrorCode::EmptyPushConstantBlock,
                            "Push-constant block is empty",
                            stage,
                            pushConstant.name
                        );

                    if (rawSize > std::numeric_limits<uint32_t>::max())
                        return fail(
                            ShaderReflectionErrorCode::PushConstantSizeOverflow,
                            "Push-constant block size overflows uint32_t",
                            stage,
                            pushConstant.name,
                            std::nullopt,
                            std::nullopt,
                            std::numeric_limits<uint32_t>::max(),
                            rawSize
                        );

                    const auto size = static_cast<uint32_t>(rawSize);
                    if (size % 4 != 0)
                        return fail(
                            ShaderReflectionErrorCode::PushConstantAlignmentInvalid,
                            "Push-constant block size is not a multiple of four",
                            stage,
                            pushConstant.name,
                            std::nullopt,
                            std::nullopt,
                            std::nullopt,
                            size
                        );

                    shader->pushConstantSize = std::max(shader->pushConstantSize, size);
                    shader->pushConstantStages |= stage;
                }

                auto reflectResources = [&]<typename ResourceRange>(
                    const ResourceRange& reflectedResources,
                    const ShaderUniformType type
                ) -> std::expected<void, ShaderReflectionError> {
                    for (const auto& resource : reflectedResources) {
                        const auto result = addUniform(resource, type);
                        if (!result)
                            return std::unexpected(result.error());
                    }
                    return {};
                };

                if (auto result = reflectResources(resources.separate_samplers, ShaderUniformType::Sampler); !result)
                    return result;
                if (auto result = reflectResources(resources.separate_images, ShaderUniformType::SampledImage); !result)
                    return result;
                if (auto result = reflectResources(resources.sampled_images, ShaderUniformType::CombinedImageSampler); !result)
                    return result;
                if (auto result = reflectResources(resources.storage_images, ShaderUniformType::StorageImage); !result)
                    return result;

                for (const auto& resource : resources.uniform_buffers) {
                    const auto& type = compiler.get_type(resource.base_type_id);
                    const uint64_t rawSize = compiler.get_declared_struct_size(type);
                    if (rawSize > std::numeric_limits<uint32_t>::max())
                        return fail(
                            ShaderReflectionErrorCode::DescriptorSizeConflict,
                            "Uniform buffer size overflows uint32_t",
                            stage,
                            resource.name,
                            std::nullopt,
                            std::nullopt,
                            std::numeric_limits<uint32_t>::max(),
                            rawSize
                        );

                    const auto result = addUniform(resource, ShaderUniformType::UniformBuffer, static_cast<uint32_t>(rawSize));
                    if (!result)
                        return std::unexpected(result.error());
                }

                for (const auto& resource : resources.storage_buffers) {
                    const auto& type = compiler.get_type(resource.base_type_id);
                    const uint64_t rawSize = compiler.get_declared_struct_size(type);
                    if (rawSize > std::numeric_limits<uint32_t>::max())
                        return fail(
                            ShaderReflectionErrorCode::DescriptorSizeConflict,
                            "Storage buffer size overflows uint32_t",
                            stage,
                            resource.name,
                            std::nullopt,
                            std::nullopt,
                            std::numeric_limits<uint32_t>::max(),
                            rawSize
                        );

                    const auto result = addUniform(resource, ShaderUniformType::StorageBuffer, static_cast<uint32_t>(rawSize));
                    if (!result)
                        return std::unexpected(result.error());
                }

                if (auto result = reflectResources(resources.subpass_inputs, ShaderUniformType::InputAttachment); !result)
                    return result;

                shader->stages |= stage;
            } catch (const spirv_cross::CompilerError& exception) {
                return fail(
                    ShaderReflectionErrorCode::SpirvReflectionFailed,
                    std::format("SPIR-V reflection failed: {}", exception.what()),
                    stage
                );
            }

        }

        std::ranges::sort(
            shader->uniformSets,
            [](const ShaderUniform& left, const ShaderUniform& right) {
                if (left.set != right.set)
                    return left.set < right.set;
                return left.binding < right.binding;
            }
        );

        return {};
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
