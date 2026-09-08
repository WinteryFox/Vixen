#include "RenderingDeviceDriver.h"

#include <algorithm>
#include <cstring>
#include <format>
#include <limits>
#include <optional>
#include <ranges>

#ifdef DEBUG_ENABLED
#include <GlslangToSpv.h>
#endif

#include <disassemble.h>
#include <spirv_cross.hpp>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <spdlog/spdlog.h>

#include "IndexFormat.h"
#include "buffer/Buffer.h"
#include "command/CommandBuffer.h"
#include "command/CommandPool.h"
#include "buffer/BufferCopyRegion.h"
#include "buffer/BufferImageCopyRegion.h"
#include "image/Image.h"
#include "image/ImageCopyRegion.h"
#include "pipeline/GraphicsPipeline.h"
#include "rendering/AttachmentInfo.h"
#include "core/error/CantCreateError.h"
#include "core/error/Macros.h"
#include "core/error/Shader.h"
#include "core/shader/Shader.h"
#include "core/shader/ShaderUniform.h"
#include "core/shader/ShaderUniformType.h"

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
            switch (stageData.stage) {
                case ShaderStageBits::Vertex:
                case ShaderStageBits::Fragment:
                case ShaderStageBits::TesselationControl:
                case ShaderStageBits::TesselationEvaluation:
                case ShaderStageBits::Compute:
                case ShaderStageBits::Geometry:
                    break;
                default:
                    return fail(
                        ShaderReflectionErrorCode::InvalidShaderStage,
                        "Shader stage is not a recognized single stage bit",
                        stageData.stage
                    );
            }

            if (stageData.entryPoint.empty())
                return fail(
                    ShaderReflectionErrorCode::EmptyEntryPointName,
                    "Shader stage specifies an empty entry-point name",
                    stageData.stage
                );

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

        if (hasGraphics && !suppliedStages.contains(ShaderStageBits::Vertex))
            return fail(
                ShaderReflectionErrorCode::MissingRequiredShaderStage,
                "A graphics shader requires a vertex stage",
                ShaderStageBits::Vertex
            );

        if (suppliedStages.contains(ShaderStageBits::TesselationControl) !=
            suppliedStages.contains(ShaderStageBits::TesselationEvaluation))
            return fail(
                ShaderReflectionErrorCode::MissingRequiredShaderStage,
                "Tessellation control and evaluation stages must be supplied together"
            );

        struct PushConstantMemberLayout {
            uint32_t offset;
            uint64_t size;
            spirv_cross::SPIRType::BaseType baseType;
            uint32_t width;
            uint32_t vectorSize;
            uint32_t columns;
            std::vector<uint32_t> arrayDimensions;
            std::optional<uint32_t> arrayStride;
            std::optional<uint32_t> matrixStride;
            bool rowMajor;

            bool operator==(const PushConstantMemberLayout&) const = default;
        };

        struct PushConstantLayout {
            uint32_t size;
            std::vector<PushConstantMemberLayout> members;

            bool operator==(const PushConstantLayout&) const = default;
        };

        std::optional<PushConstantLayout> reflectedPushConstantLayout;

        for (const auto& stageData : stages) {
            const auto stage = stageData.stage;
            const auto& spirv = stageData.spirv;
            const auto& entryPoint = stageData.entryPoint;

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
                auto compiler = spirv_cross::Compiler(words);

                const auto entryPoints = compiler.get_entry_points_and_stages();
                if (entryPoints.empty())
                    return fail(ShaderReflectionErrorCode::NoEntryPoint, "SPIR-V module has no entry point", stage);

                const auto expectedExecutionModel = [&]() -> std::optional<spv::ExecutionModel> {
                    switch (stage) {
                        case ShaderStageBits::Vertex:
                            return spv::ExecutionModelVertex;
                        case ShaderStageBits::Fragment:
                            return spv::ExecutionModelFragment;
                        case ShaderStageBits::TesselationControl:
                            return spv::ExecutionModelTessellationControl;
                        case ShaderStageBits::TesselationEvaluation:
                            return spv::ExecutionModelTessellationEvaluation;
                        case ShaderStageBits::Compute:
                            return spv::ExecutionModelGLCompute;
                        case ShaderStageBits::Geometry:
                            return spv::ExecutionModelGeometry;
                        default:
                            return std::nullopt;
                    }
                }();

                if (!expectedExecutionModel)
                    return fail(
                        ShaderReflectionErrorCode::EntryPointStageMismatch,
                        "Cannot determine an SPIR-V execution model for the supplied shader stage",
                        stage,
                        entryPoint
                    );

                const auto selectedEntryPoint = std::ranges::find_if(
                    entryPoints,
                    [&](const spirv_cross::EntryPoint& candidate) {
                        return candidate.name == entryPoint &&
                            candidate.execution_model == *expectedExecutionModel;
                    }
                );

                if (selectedEntryPoint == entryPoints.end()) {
                    const auto namedEntryPoint = std::ranges::find_if(
                        entryPoints,
                        [&](const spirv_cross::EntryPoint& candidate) {
                            return candidate.name == entryPoint;
                        }
                    );

                    if (namedEntryPoint == entryPoints.end())
                        return fail(
                            ShaderReflectionErrorCode::EntryPointNotFound,
                            std::format(
                                "SPIR-V module does not contain the requested entry point '{}'",
                                entryPoint
                            ),
                            stage,
                            entryPoint
                        );

                    return fail(
                        ShaderReflectionErrorCode::EntryPointStageMismatch,
                        std::format(
                            "SPIR-V entry point '{}' does not match its supplied shader stage",
                            entryPoint
                        ),
                        stage,
                        entryPoint,
                        std::nullopt,
                        std::nullopt,
                        static_cast<uint64_t>(*expectedExecutionModel),
                        static_cast<uint64_t>(namedEntryPoint->execution_model)
                    );
                }

                compiler.set_entry_point(entryPoint, *expectedExecutionModel);
                const auto activeVariables = compiler.get_active_interface_variables();
                const auto resources = compiler.get_shader_resources(activeVariables);

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

                auto addUniform = [&](
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
                                               ? std::optional(
                                                   compiler.get_decoration(resource.id, spv::DecorationBinding))
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

                    if (existing->stages.contains(stage))
                        return std::unexpected(
                            ShaderReflectionError{
                                .type = ShaderReflectionErrorCode::DuplicateDescriptorBinding,
                                .stages = stage,
                                .resourceName = resource.name,
                                .set = uniform.set,
                                .binding = uniform.binding,
                                .expected = std::nullopt,
                                .actual = std::nullopt,
                                .detail = "Shader stage declares the same descriptor binding more than once"
                            }
                        );

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

                    PushConstantLayout layout{
                        .size = size,
                        .members = {}
                    };
                    layout.members.reserve(type.member_types.size());
                    for (uint32_t member = 0; member < type.member_types.size(); ++member) {
                        const auto& memberType = compiler.get_type(type.member_types[member]);
                        layout.members.push_back({
                            .offset = compiler.type_struct_member_offset(type, member),
                            .size = compiler.get_declared_struct_member_size(type, member),
                            .baseType = memberType.basetype,
                            .width = memberType.width,
                            .vectorSize = memberType.vecsize,
                            .columns = memberType.columns,
                            .arrayDimensions = {memberType.array.begin(), memberType.array.end()},
                            .arrayStride = !memberType.array.empty()
                                               ? std::optional(compiler.type_struct_member_array_stride(type, member))
                                               : std::nullopt,
                            .matrixStride = compiler.has_member_decoration(type.self, member,
                                                                           spv::DecorationMatrixStride)
                                                ? std::optional(compiler.get_member_decoration(type.self, member,
                                                    spv::DecorationMatrixStride))
                                                : std::nullopt,
                            .rowMajor = compiler.has_member_decoration(type.self, member, spv::DecorationRowMajor)
                        });
                    }

                    if (reflectedPushConstantLayout && *reflectedPushConstantLayout != layout)
                        return fail(
                            ShaderReflectionErrorCode::PushConstantLayoutConflict,
                            "Push-constant layout conflicts with another shader stage",
                            stage,
                            pushConstant.name,
                            std::nullopt,
                            std::nullopt,
                            reflectedPushConstantLayout->size,
                            layout.size
                        );

                    reflectedPushConstantLayout = std::move(layout);
                    shader->pushConstantSize = size;
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

                for (const auto& resource : resources.separate_images) {
                    const auto& resourceType = compiler.get_type(resource.type_id);
                    const auto uniformType = resourceType.image.dim == spv::DimBuffer
                                                 ? ShaderUniformType::UniformTexelBuffer
                                                 : ShaderUniformType::SampledImage;
                    if (auto result = addUniform(resource, uniformType); !result)
                        return result;
                }

                for (const auto& resource : resources.sampled_images) {
                    const auto& resourceType = compiler.get_type(resource.type_id);
                    const auto uniformType = resourceType.image.dim == spv::DimBuffer
                                                 ? ShaderUniformType::UniformTexelBuffer
                                                 : ShaderUniformType::CombinedImageSampler;
                    if (auto result = addUniform(resource, uniformType); !result)
                        return result;
                }

                for (const auto& resource : resources.storage_images) {
                    const auto& resourceType = compiler.get_type(resource.type_id);
                    const auto uniformType = resourceType.image.dim == spv::DimBuffer
                                                 ? ShaderUniformType::StorageTexelBuffer
                                                 : ShaderUniformType::StorageImage;
                    if (auto result = addUniform(resource, uniformType); !result)
                        return result;
                }

                for (const auto& resource : resources.uniform_buffers) {
                    const auto& type = compiler.get_type(resource.base_type_id);
                    const uint64_t rawSize = compiler.get_declared_struct_size(type);
                    if (rawSize > std::numeric_limits<uint32_t>::max())
                        return fail(
                            ShaderReflectionErrorCode::DescriptorSizeOverflow,
                            "Uniform buffer size overflows uint32_t",
                            stage,
                            resource.name,
                            std::nullopt,
                            std::nullopt,
                            std::numeric_limits<uint32_t>::max(),
                            rawSize
                        );

                    const auto result = addUniform(resource, ShaderUniformType::UniformBuffer,
                                                   static_cast<uint32_t>(rawSize));
                    if (!result)
                        return std::unexpected(result.error());
                }

                for (const auto& resource : resources.storage_buffers) {
                    const auto& type = compiler.get_type(resource.base_type_id);
                    const uint64_t rawSize = compiler.get_declared_struct_size(type);
                    if (rawSize > std::numeric_limits<uint32_t>::max())
                        return fail(
                            ShaderReflectionErrorCode::DescriptorSizeOverflow,
                            "Storage buffer size overflows uint32_t",
                            stage,
                            resource.name,
                            std::nullopt,
                            std::nullopt,
                            std::numeric_limits<uint32_t>::max(),
                            rawSize
                        );

                    const auto result = addUniform(resource, ShaderUniformType::StorageBuffer,
                                                   static_cast<uint32_t>(rawSize));
                    if (!result)
                        return std::unexpected(result.error());
                }

                if (auto result = reflectResources(resources.subpass_inputs, ShaderUniformType::InputAttachment); !
                    result)
                    return result;

                const spirv_cross::Resource* unsupportedResource = nullptr;
                if (!resources.atomic_counters.empty())
                    unsupportedResource = &resources.atomic_counters.front();
                else if (!resources.acceleration_structures.empty())
                    unsupportedResource = &resources.acceleration_structures.front();
                else if (!resources.gl_plain_uniforms.empty())
                    unsupportedResource = &resources.gl_plain_uniforms.front();
                else if (!resources.shader_record_buffers.empty())
                    unsupportedResource = &resources.shader_record_buffers.front();

                if (unsupportedResource != nullptr)
                    return fail(
                        ShaderReflectionErrorCode::UnsupportedResourceType,
                        "SPIR-V module contains a resource type unsupported by Vixen",
                        stage,
                        unsupportedResource->name
                    );

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

    namespace {
        auto commandError(CommandErrorCode code, std::string_view operation, std::string_view detail)
            -> std::unexpected<CommandError> {
            return std::unexpected{CommandError{code, std::format("{}: {}", operation, detail)}};
        }

        auto invalidArgument(std::string_view operation, std::string_view detail)
            -> std::unexpected<CommandError> {
            return commandError(CommandErrorCode::InvalidArgument, operation, detail);
        }

        bool validRange(uint64_t offset, uint64_t size, uint64_t capacity) {
            return size != 0 && offset < capacity && size <= capacity - offset;
        }

        auto checkImageRange(const Image* image, const ImageSubresourceRange& range, std::string_view operation)
            -> std::expected<void, CommandError> {
            if (image == nullptr)
                return invalidArgument(operation, "image is null");
            if (range.aspect.empty() || (range.aspect.value() & ~getImageAspects(image->format.format).value()) != 0)
                return invalidArgument(operation, "subresource aspects do not match the image format");
            if (!validRange(range.baseMipmap, range.mipmapCount, image->format.mipmapCount) ||
                !validRange(range.baseLayer, range.layerCount, image->format.layerCount))
                return invalidArgument(operation, "subresource mip or layer range is outside the image");
            return {};
        }

        auto checkImageRegion(const Image* image, const ImageSubresourceLayers& layers,
                              glm::ivec3 offset, glm::uvec3 extent, std::string_view operation)
            -> std::expected<void, CommandError> {
            if (auto result = checkImageRange(image, {
                                                  layers.aspect, layers.mipmap, 1, layers.baseLayer, layers.layerCount
                                              }, operation); !result)
                return result;
            if (layers.mipmap >= 32)
                return invalidArgument(operation, "mip level exceeds the supported dimension width");
            const glm::uvec3 dimensions{
                std::max(1u, image->format.width >> layers.mipmap),
                std::max(1u, image->format.height >> layers.mipmap),
                std::max(1u, image->format.depth >> layers.mipmap)
            };
            for (int axis = 0; axis < 3; ++axis)
                if (offset[axis] < 0 || !validRange(static_cast<uint32_t>(offset[axis]), extent[axis],
                                                    dimensions[axis]))
                    return invalidArgument(operation, "copy region extends outside the image mip");
            return {};
        }

        bool isTransferLayout(ImageLayout layout, bool source) {
            return layout == ImageLayout::General || layout == ImageLayout::StorageOptimal ||
            (source
                 ? (layout == ImageLayout::CopySourceOptimal || layout == ImageLayout::ResolveSourceOptimal)
                 : (layout == ImageLayout::CopyDestinationOptimal || layout == ImageLayout::ResolveDestinationOptimal));
        }
    }

    auto RenderingDeviceDriver::checkRecording(
        const CommandBuffer* commandBuffer,
        const std::string_view operation,
        const QueueFamilyFlags allowedQueues,
        const RenderingScope scope
    ) -> std::expected<void, CommandError> {
        if (commandBuffer == nullptr)
            return invalidArgument(operation, "command buffer is null");

        if (commandBuffer->getState() != CommandBuffer::State::Recording)
            return commandError(CommandErrorCode::InvalidState, operation, "command buffer must be Recording");

        if (!allowedQueues.empty() && (commandBuffer->queueCapabilities & allowedQueues).empty())
            return commandError(CommandErrorCode::InvalidState, operation,
                                "command buffer's queue family does not support this operation");

        if (scope == RenderingScope::Outside && commandBuffer->renderingState)
            return commandError(CommandErrorCode::InvalidState, operation,
                                "command must be recorded outside a rendering scope");

        if (scope == RenderingScope::Inside && !commandBuffer->renderingState)
            return commandError(CommandErrorCode::InvalidState, operation, "no rendering scope is active");

        return {};
    }

    auto RenderingDeviceDriver::beginCommandBuffer(
        CommandBuffer* commandBuffer
    ) -> std::expected<void, CommandError> {
        constexpr std::string_view operation = "beginCommandBuffer";

        if (commandBuffer == nullptr)
            return invalidArgument(operation, "command buffer is null");

        if (commandBuffer->getState() != CommandBuffer::State::Initial)
            return commandError(CommandErrorCode::InvalidState, operation,
                                "command buffer must be Initial; reset its pool before recording again");
        return {};
    }

    auto RenderingDeviceDriver::endCommandBuffer(
        CommandBuffer* commandBuffer
    ) -> std::expected<void, CommandError> {
        constexpr std::string_view operation = "endCommandBuffer";

        return checkRecording(commandBuffer, operation, {}, RenderingScope::Outside);
    }

    auto RenderingDeviceDriver::commandBeginRenderPass(
        CommandBuffer* commandBuffer,
        const RenderingInfo& renderingInfo
    ) -> std::expected<void, CommandError> {
        constexpr std::string_view operation = "commandBeginRenderPass";
        if (auto result = checkRecording(commandBuffer, operation, QueueFamilyBits::Graphics, RenderingScope::Outside);
            !result)
            return result;

        if (renderingInfo.extent.x == 0 || renderingInfo.extent.y == 0 || renderingInfo.layerCount == 0)
            return invalidArgument(operation, "rendering extent and layer count must be nonzero");

        if (renderingInfo.colorAttachments.size() > getMaxColorAttachments())
            return invalidArgument(operation, "color attachment count exceeds the device limit");

        std::optional<ImageSamples> samples;
        auto checkAttachment = [&](
            const AttachmentInfo& attachment,
            bool depthStencil
        )-> std::expected<void, CommandError> {
            if (attachment.image == nullptr)
                return invalidArgument(operation, "attachment image is null");

            if (depthStencil && attachment.resolveImage != nullptr)
                return invalidArgument(operation, "depth/stencil resolve targets are not supported yet");

            const auto& format = attachment.image->format;
            const bool hasDepthStencil = hasDepthAspect(format.format) || hasStencilAspect(format.format);

            if (hasDepthStencil != depthStencil)
                return invalidArgument(operation, "attachment format does not match its color or depth/stencil role");

            const auto usage = depthStencil ? ImageUsageBits::DepthStencilAttachment : ImageUsageBits::ColorAttachment;

            if (!format.usage.contains(usage))
                return invalidArgument(operation, "attachment image was not created for its attachment usage");

            if (renderingInfo.extent.x > format.width || renderingInfo.extent.y > format.height ||
                renderingInfo.layerCount > format.layerCount)
                return invalidArgument(operation, "rendering extent or layer count exceeds an attachment");

            if (samples && *samples != format.samples)
                return invalidArgument(operation, "all rendering attachments must have the same sample count");
            samples = format.samples;

            if (attachment.layout != ImageLayout::General &&
                attachment.layout != (depthStencil
                                          ? ImageLayout::DepthStencilAttachmentOptimal
                                          : ImageLayout::ColorAttachmentOptimal) &&
                !(depthStencil && attachment.layout == ImageLayout::DepthStencilReadOnlyOptimal))
                return invalidArgument(operation, "attachment layout is not valid for its attachment role");

            if (depthStencil && attachment.layout == ImageLayout::DepthStencilReadOnlyOptimal && attachment.loadAction
                == LoadAction::Clear)
                return invalidArgument(
                    operation, "a read-only depth/stencil attachment cannot use a clear load action");

            if (attachment.loadAction != LoadAction::Load && attachment.loadAction != LoadAction::Clear &&
                attachment.loadAction != LoadAction::DontCare)
                return invalidArgument(operation, "attachment load action contains an unrecognized value");

            if (attachment.storeAction != StoreAction::Store && attachment.storeAction != StoreAction::DontCare &&
                attachment.storeAction != StoreAction::Resolve && attachment.storeAction !=
                StoreAction::StoreAndResolve)
                return invalidArgument(operation, "attachment store action contains an unrecognized value");

            return {};
        };

        for (const auto& attachment : renderingInfo.colorAttachments)
            if (auto result = checkAttachment(attachment, false); !result)
                return result;

        if (renderingInfo.depthStencilAttachment)
            return checkAttachment(*renderingInfo.depthStencilAttachment, true);

        return {};
    }

    auto RenderingDeviceDriver::commandEndRenderPass(
        CommandBuffer* commandBuffer
    ) -> std::expected<void, CommandError> {
        constexpr std::string_view operation = "commandEndRenderPass";

        return checkRecording(
            commandBuffer,
            operation,
            QueueFamilyBits::Graphics,
            RenderingScope::Inside
        );
    }

    auto RenderingDeviceDriver::commandSetViewport(
        CommandBuffer* commandBuffer,
        const std::vector<glm::uvec2>& viewports
    ) -> std::expected<void, CommandError> {
        constexpr std::string_view operation = "commandSetViewport";
        if (auto result = checkRecording(
            commandBuffer,
            operation,
            QueueFamilyBits::Graphics
        ); !result)
            return result;

        if (viewports.empty() ||
            viewports.size() > std::numeric_limits<uint32_t>::max())
            return invalidArgument(operation, "viewport count must be nonzero and representable as uint32_t");

        for (const auto& viewport : viewports)
            if (viewport.x == 0 ||
                viewport.y == 0)
                return invalidArgument(
                    operation,
                    "viewport dimensions must be nonzero"
                );

        return {};
    }

    auto RenderingDeviceDriver::commandSetScissor(
        CommandBuffer* commandBuffer,
        const std::vector<glm::uvec2>& scissors
    ) -> std::expected<void, CommandError> {
        constexpr std::string_view operation = "commandSetScissor";

        if (auto result = checkRecording(
            commandBuffer,
            operation,
            QueueFamilyBits::Graphics
        ); !result)
            return result;

        if (scissors.empty() ||
            scissors.size() > std::numeric_limits<uint32_t>::max())
            return invalidArgument(operation, "scissor count must be nonzero and representable as uint32_t");

        for (const auto& scissor : scissors)
            if (scissor.x > static_cast<uint32_t>(std::numeric_limits<int32_t>::max()) ||
                scissor.y > static_cast<uint32_t>(std::numeric_limits<int32_t>::max()))
                return invalidArgument(operation, "scissor coordinates must fit in int32_t");

        return {};
    }

    auto RenderingDeviceDriver::commandSetBlendConstants(
        CommandBuffer* commandBuffer,
        glm::vec4 blendConstants
    ) -> std::expected<void, CommandError> {
        constexpr std::string_view operation = "commandSetBlendConstants";
        (void)blendConstants;

        return checkRecording(commandBuffer, operation, QueueFamilyBits::Graphics);
    }

    auto RenderingDeviceDriver::commandBindVertexBuffers(
        CommandBuffer* commandBuffer,
        const std::vector<const Buffer*>& buffers,
        const std::vector<uint64_t>& offsets
    ) -> std::expected<void, CommandError> {
        constexpr std::string_view operation = "commandBindVertexBuffers";
        if (auto result = checkRecording(
            commandBuffer,
            operation,
            QueueFamilyBits::Graphics
        ); !result)
            return result;

        if (buffers.empty() || buffers.size() != offsets.size())
            return invalidArgument(operation, "buffer and offset arrays must have the same nonzero length");

        if (buffers.size() > getMaxVertexInputBindings())
            return invalidArgument(operation, "binding count exceeds the device's vertex-buffer binding limit");

        for (size_t i = 0; i < buffers.size(); ++i) {
            if (buffers[i] == nullptr)
                return invalidArgument(
                    operation,
                    std::format(
                        "vertex buffer at binding {} is null",
                        i
                    )
                );

            if (!buffers[i]->getUsage().contains(BufferUsageBits::Vertex))
                return invalidArgument(
                    operation, std::format(
                        "buffer at binding {} was not created for Vertex usage",
                        i
                    )
                );

            if (offsets[i] >= buffers[i]->getSize())
                return invalidArgument(
                    operation,
                    std::format(
                        "binding {} offset {} must be less than buffer size {}",
                        i,
                        offsets[i],
                        buffers[i]->getSize()
                    )
                );
        }
        return {};
    }

    auto RenderingDeviceDriver::commandBindIndexBuffers(
        CommandBuffer* commandBuffer,
        const Buffer* buffer,
        const IndexFormat format,
        uint64_t offset
    ) -> std::expected<void, CommandError> {
        constexpr std::string_view operation = "commandBindIndexBuffers";
        if (auto result = checkRecording(
            commandBuffer,
            operation,
            QueueFamilyBits::Graphics
        ); !result)
            return result;

        if (buffer == nullptr)
            return invalidArgument(
                operation,
                "index buffer is null"
            );

        if (!buffer->getUsage().contains(BufferUsageBits::Index))
            return invalidArgument(
                operation,
                "buffer was not created for Index usage"
            );

        uint64_t indexSize = 0;
        switch (format) {
            case IndexFormat::UnsignedInt16:
                indexSize = sizeof(uint16_t);
                break;

            case IndexFormat::UnsignedInt32:
                indexSize = sizeof(uint32_t);
                break;
        }

        if (indexSize == 0)
            return invalidArgument(
                operation,
                "index format contains an unrecognized value"
            );

        if (offset >= buffer->getSize())
            return invalidArgument(
                operation,
                std::format(
                    "index-buffer offset {} must be less than buffer size {}",
                    offset,
                    buffer->getSize()
                )
            );

        if (offset % indexSize != 0)
            return invalidArgument(
                operation,
                std::format(
                    "index-buffer offset {} must be aligned to {} bytes",
                    offset,
                    indexSize
                )
            );

        return {};
    }

    auto RenderingDeviceDriver::commandBindGraphicsPipeline(
        CommandBuffer* commandBuffer,
        const GraphicsPipeline* pipeline
    ) -> std::expected<void, CommandError> {
        if (auto result = checkRecording(
            commandBuffer,
            "commandBindGraphicsPipeline",
            QueueFamilyBits::Graphics,
            RenderingScope::Inside
        ); !result)
            return result;

        (void)pipeline;

        return {};
    }

    auto RenderingDeviceDriver::commandBindComputePipeline(
        CommandBuffer* commandBuffer,
        const ComputePipeline* pipeline
        ) -> std::expected<void, CommandError> {
        if (auto result = checkRecording(
            commandBuffer,
            "commandBindComputePipeline",
            QueueFamilyBits::Compute,
            RenderingScope::Inside
        ); !result)
            return result;

        (void)pipeline;

        return {};
    }

    auto RenderingDeviceDriver::checkGraphicsDrawState(
        const CommandBuffer* commandBuffer,
        const std::string_view operation
    ) -> std::expected<void, CommandError> {
        if (auto result = checkRecording(
            commandBuffer,
            operation,
            QueueFamilyBits::Graphics,
            RenderingScope::Inside
        ); !result)
            return result;

        if (commandBuffer->boundGraphicsPipeline == nullptr)
            return commandError(
                CommandErrorCode::InvalidState,
                operation,
                "no graphics pipeline is bound"
            );

        const auto& pipeline = commandBuffer->boundGraphicsPipeline->state;
        const auto& rendering = *commandBuffer->renderingState;
        if (pipeline.colorFormats.size() != rendering.colorFormats.size())
            return commandError(
                CommandErrorCode::InvalidState,
                operation,
                std::format(
                    "graphics pipeline declares {} color attachments, but the active rendering scope has {}",
                    pipeline.colorFormats.size(),
                    rendering.colorFormats.size()
                )
            );

        for (size_t index = 0; index < pipeline.colorFormats.size(); ++index)
            if (pipeline.colorFormats[index] != rendering.colorFormats[index])
                return commandError(
                    CommandErrorCode::InvalidState,
                    operation,
                    std::format(
                        "graphics pipeline color format at attachment {} does not match the active rendering scope",
                        index
                    )
                );

        if (pipeline.depthStencilFormat != rendering.depthStencilFormat)
            return commandError(
                CommandErrorCode::InvalidState,
                operation,
                "graphics pipeline depth/stencil format does not match the active rendering scope"
            );

        if ((!rendering.colorFormats.empty() ||
                rendering.depthStencilFormat) &&
            pipeline.multisampling.samples != rendering.samples)
            return commandError(
                CommandErrorCode::InvalidState,
                operation,
                "graphics pipeline sample count does not match the active rendering attachments"
            );

        for (const auto state : {DynamicStateBits::Viewport, DynamicStateBits::Scissor})
            if (pipeline.dynamicStates.contains(state) && !commandBuffer->dynamicStates.contains(state))
                return commandError(
                    CommandErrorCode::InvalidState,
                    operation,
                    std::format(
                        "the bound graphics pipeline requires {} to be set before drawing",
                        state == DynamicStateBits::Viewport ? "a viewport" : "a scissor"
                    )
                );

        const auto usesConstant = [](const BlendFactor factor) {
            return factor == BlendFactor::ConstantColor ||
                factor == BlendFactor::OneMinusConstantColor ||
                factor == BlendFactor::ConstantAlpha ||
                factor == BlendFactor::OneMinusConstantAlpha;
        };

        if (!pipeline.rasterization.isRasterizerDiscardEnabled &&
            pipeline.dynamicStates.contains(DynamicStateBits::BlendConstants) &&
            !commandBuffer->dynamicStates.contains(DynamicStateBits::BlendConstants))
            for (const auto& blending : pipeline.colorBlending)
                if (blending.isEnabled &&
                    (usesConstant(blending.sourceColorBlendFactor) ||
                        usesConstant(blending.destinationColorBlendFactor) ||
                        usesConstant(blending.sourceAlphaBlendFactor) ||
                        usesConstant(blending.destinationAlphaBlendFactor)))
                    return commandError(
                        CommandErrorCode::InvalidState,
                        operation,
                        "the bound graphics pipeline uses constant blend factors; set blend constants before drawing"
                    );
        return {};
    }

    auto RenderingDeviceDriver::checkVertexBindings(
        const CommandBuffer* commandBuffer,
        const std::string_view operation,
        const uint32_t count,
        const uint32_t instanceCount,
        const std::optional<uint32_t> firstVertex,
        const uint32_t firstInstance
    ) -> std::expected<void, CommandError> {
        const auto& pipeline = commandBuffer->boundGraphicsPipeline->state;
        for (const auto& attribute : pipeline.vertexAttributes) {
            if (attribute.binding >= commandBuffer->vertexBindings.size() ||
                commandBuffer->vertexBindings[attribute.binding].buffer == nullptr)
                return commandError(
                    CommandErrorCode::InvalidState,
                    operation,
                    std::format(
                        "vertex attribute {} requires a buffer at binding {}",
                        attribute.location, attribute.binding
                    )
                );

            const auto& bound = commandBuffer->vertexBindings[attribute.binding];
            if (!bound.buffer->getUsage().contains(BufferUsageBits::Vertex) ||
                bound.offset >= bound.buffer->getSize())
                return commandError(
                    CommandErrorCode::InvalidState,
                    operation,
                    std::format(
                        "vertex binding {} requires a Vertex-usage buffer with an in-bounds offset",
                        attribute.binding
                    )
                );

            const auto binding = std::ranges::find(pipeline.vertexBindings, attribute.binding,
                                                   &VertexBindingDescription::binding);
            if (binding == pipeline.vertexBindings.end())
                return commandError(
                    CommandErrorCode::InvalidState,
                    operation,
                    std::format(
                        "vertex attribute {} references undeclared pipeline binding {}",
                        attribute.location,
                        attribute.binding
                    )
                );

            if (count == 0 || instanceCount == 0)
                continue;

            const bool knownElement = binding->rate == InputRate::Instance || firstVertex.has_value();
            const uint64_t lastElement = binding->rate == InputRate::Instance
                                             ? static_cast<uint64_t>(firstInstance) + instanceCount - 1
                                             : (firstVertex ? static_cast<uint64_t>(*firstVertex) + count - 1 : 0);
            const uint64_t available = bound.buffer->getSize() - bound.offset;
            const uint64_t attributeEnd = static_cast<uint64_t>(attribute.offset) + getTexelSize(attribute.format);

            if (attributeEnd > available ||
                (knownElement &&
                    binding->stride != 0 &&
                    lastElement > (available - attributeEnd) / binding->stride))
                return invalidArgument(
                    operation,
                    std::format(
                        "draw range exceeds vertex buffer binding {} for attribute {} (last {} element {}, stride {}, available bytes {})",
                        attribute.binding,
                        attribute.location,
                        binding->rate == InputRate::Instance ? "instance" : "vertex",
                        knownElement ? std::to_string(lastElement) : "GPU-selected",
                        binding->stride,
                        available
                    )
                );
        }
        return {};
    }

    auto RenderingDeviceDriver::commandDraw(
        CommandBuffer* commandBuffer,
        const uint32_t vertexCount,
        const uint32_t instanceCount,
        const uint32_t firstVertex,
        const uint32_t firstInstance
    ) -> std::expected<void, CommandError> {
        constexpr std::string_view operation = "commandDraw";
        if (auto result = checkGraphicsDrawState(commandBuffer, operation); !result)
            return result;

        return checkVertexBindings(commandBuffer, operation, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    auto RenderingDeviceDriver::commandDrawIndexed(
        CommandBuffer* commandBuffer,
        const uint32_t indexCount,
        const uint32_t instanceCount,
        const uint32_t firstIndex,
        const int32_t vertexOffset,
        const uint32_t firstInstance
    ) -> std::expected<void, CommandError> {
        constexpr std::string_view operation = "commandDrawIndexed";
        if (auto result = checkGraphicsDrawState(commandBuffer, operation); !result)
            return result;

        if (!commandBuffer->indexBinding || commandBuffer->indexBinding->buffer == nullptr)
            return commandError(CommandErrorCode::InvalidState, operation, "no index buffer is bound");

        const auto& binding = *commandBuffer->indexBinding;
        uint64_t indexSize = 0;
        switch (binding.format) {
            case IndexFormat::UnsignedInt16:
                indexSize = sizeof(uint16_t);
                break;

            case IndexFormat::UnsignedInt32:
                indexSize = sizeof(uint32_t);
                break;
        }

        if (indexSize == 0 ||
            !binding.buffer->getUsage().contains(BufferUsageBits::Index) ||
            binding.offset >= binding.buffer->getSize() ||
            binding.offset % indexSize != 0)
            return commandError(
                CommandErrorCode::InvalidState,
                operation,
                "index binding requires an Index-usage buffer, a supported format, and an aligned in-bounds offset"
            );

        const uint64_t availableIndices = (binding.buffer->getSize() - binding.offset) / indexSize;
        const uint64_t endIndex = static_cast<uint64_t>(firstIndex) + indexCount;
        if (endIndex > availableIndices)
            return invalidArgument(
                operation,
                std::format(
                    "index range [{}, {}) exceeds the {} indices available after binding offset {}",
                    firstIndex, endIndex, availableIndices, binding.offset)
            );

        (void)vertexOffset;

        return checkVertexBindings(commandBuffer, operation, indexCount, instanceCount, std::nullopt, firstInstance);
    }

    auto RenderingDeviceDriver::commandDispatch(
        CommandBuffer* commandBuffer,
        const uint32_t groupCountX,
        const uint32_t groupCountY,
        const uint32_t groupCountZ
    ) -> std::expected<void, CommandError> {
        if (auto result = checkRecording(
            commandBuffer,
            "commandDispatch",
            QueueFamilyBits::Compute,
            RenderingScope::Outside
        ); !result)
            return result;

        if (commandBuffer->boundComputePipeline == nullptr)
            return commandError(CommandErrorCode::InvalidState, "commandDispatch", "no compute pipeline is bound");

        (void)groupCountX;
        (void)groupCountY;
        (void)groupCountZ;

        return {};
    }

    auto RenderingDeviceDriver::commandPipelineBarrier(
        CommandBuffer* commandBuffer,
        PipelineStageFlags sourceStages,
        PipelineStageFlags destinationStages,
        const std::vector<MemoryBarrier>& memoryBarriers,
        const std::vector<BufferBarrier>& bufferBarriers,
        const std::vector<ImageBarrier>& imageBarriers
    ) -> std::expected<void, CommandError> {
        constexpr std::string_view operation = "commandPipelineBarrier";
        if (auto result = checkRecording(commandBuffer, operation, {}, RenderingScope::Outside); !result)
            return result;
        const auto graphicsStages = PipelineStageBits::VertexInput | PipelineStageBits::VertexShader |
            PipelineStageBits::TessellationControl | PipelineStageBits::TessellationEvaluation |
            PipelineStageBits::GeometryShader | PipelineStageBits::FragmentShader |
            PipelineStageBits::EarlyFragmentTests | PipelineStageBits::LateFragmentTests |
            PipelineStageBits::ColorAttachmentOutput | PipelineStageBits::Resolve | PipelineStageBits::AllGraphics;
        const auto shaderQueues = QueueFamilyBits::Graphics | QueueFamilyBits::Compute;
        for (auto stages : {sourceStages, destinationStages}) {
            if ((stages.value() & ~((1u << 17) - 1)) != 0)
                return invalidArgument(operation, "pipeline stage mask contains unrecognized bits");
            if (!(stages & graphicsStages).empty() && !commandBuffer->queueCapabilities.contains(
                QueueFamilyBits::Graphics))
                return commandError(CommandErrorCode::InvalidState, operation,
                                    "graphics stages require a graphics-capable command buffer");
            if (stages.contains(PipelineStageBits::ComputeShader) && !commandBuffer->queueCapabilities.contains(
                QueueFamilyBits::Compute))
                return commandError(CommandErrorCode::InvalidState, operation,
                                    "compute stages require a compute-capable command buffer");
            if (stages.contains(PipelineStageBits::DrawIndirect) && (commandBuffer->queueCapabilities & shaderQueues).
                empty())
                return commandError(CommandErrorCode::InvalidState, operation,
                                    "indirect stages require graphics or compute capability");
            if (stages.contains(PipelineStageBits::Copy) && !commandBuffer->queueCapabilities.contains(
                QueueFamilyBits::Transfer))
                return commandError(CommandErrorCode::InvalidState, operation,
                                    "copy stages require transfer capability");
        }
        for (const auto& barrier : bufferBarriers)
            if (barrier.buffer == nullptr || !validRange(barrier.offset, barrier.size, barrier.buffer->getSize()))
                return invalidArgument(operation, "buffer barrier has a null buffer or an invalid byte range");
        for (const auto& barrier : imageBarriers)
            if (auto result = checkImageRange(barrier.image, barrier.subresources, operation); !result)
                return result;
            else if (barrier.newLayout == ImageLayout::Undefined)
                return invalidArgument(operation, "an image barrier cannot transition into Undefined layout");
        if (memoryBarriers.size() > std::numeric_limits<uint32_t>::max() ||
            bufferBarriers.size() > std::numeric_limits<uint32_t>::max() ||
            imageBarriers.size() > std::numeric_limits<uint32_t>::max())
            return invalidArgument(operation, "barrier count exceeds uint32_t");
        return {};
    }

    auto RenderingDeviceDriver::commandClearBuffer(
        CommandBuffer* commandBuffer,
        Buffer* buffer,
        uint64_t offset,
        uint64_t size
    ) -> std::expected<void, CommandError> {
        constexpr std::string_view operation = "commandClearBuffer";
        if (auto result = checkRecording(commandBuffer, operation, QueueFamilyBits::Transfer, RenderingScope::Outside);
            !result)
            return result;
        if (buffer == nullptr || !buffer->getUsage().contains(BufferUsageBits::CopyDestination))
            return invalidArgument(operation, "destination buffer is null or lacks CopyDestination usage");
        if (!validRange(offset, size, buffer->getSize()) || offset % 4 != 0 || size % 4 != 0)
            return invalidArgument(operation, "clear range must be nonempty, in bounds, and aligned to four bytes");
        return {};
    }

    auto RenderingDeviceDriver::commandCopyBuffer(
        CommandBuffer* commandBuffer,
        Buffer* source,
        Buffer* destination,
        const std::vector<BufferCopyRegion>& regions
    ) -> std::expected<void, CommandError> {
        constexpr std::string_view operation = "commandCopyBuffer";
        if (auto result = checkRecording(commandBuffer, operation, QueueFamilyBits::Transfer, RenderingScope::Outside);
            !result)
            return result;
        if (source == nullptr || destination == nullptr)
            return invalidArgument(operation, "source or destination buffer is null");
        if (!source->getUsage().contains(BufferUsageBits::CopySource) ||
            !destination->getUsage().contains(BufferUsageBits::CopyDestination))
            return invalidArgument(operation, "buffers lack the required copy usages");
        if (regions.empty() || regions.size() > std::numeric_limits<uint32_t>::max())
            return invalidArgument(operation, "copy region count must be nonzero and representable as uint32_t");
        for (const auto& region : regions)
            if (!validRange(region.sourceOffset, region.size, source->getSize()) ||
                !validRange(region.destinationOffset, region.size, destination->getSize()))
                return invalidArgument(operation, "copy region is empty or extends outside a buffer");
        if (source == destination)
            for (const auto& src : regions)
                for (const auto& dst : regions)
                    if (src.sourceOffset < dst.destinationOffset + dst.size && dst.destinationOffset < src.sourceOffset
                        + src.size)
                        return invalidArgument(operation, "source and destination copy ranges overlap");
        return {};
    }

    auto RenderingDeviceDriver::commandCopyImage(
        CommandBuffer* commandBuffer,
        Image* source,
        ImageLayout sourceLayout,
        Image* destination,
        ImageLayout destinationLayout,
        const std::vector<ImageCopyRegion>& regions
    ) -> std::expected<void, CommandError> {
        constexpr std::string_view operation = "commandCopyImage";
        if (auto result = checkRecording(commandBuffer, operation, QueueFamilyBits::Transfer, RenderingScope::Outside);
            !result)
            return result;
        if (source == nullptr || destination == nullptr)
            return invalidArgument(operation, "source or destination image is null");
        if (!isTransferLayout(sourceLayout, true) || !isTransferLayout(destinationLayout, false))
            return invalidArgument(operation, "image layout does not support the requested copy direction");
        if (regions.empty() || regions.size() > std::numeric_limits<uint32_t>::max())
            return invalidArgument(operation, "copy region count must be nonzero and representable as uint32_t");
        for (const auto& region : regions) {
            if (auto result = checkImageRegion(source, region.sourceSubresources, region.sourceOffset, region.size,
                                               operation); !result)
                return result;
            if (auto result = checkImageRegion(destination, region.destinationSubresources, region.destinationOffset,
                                               region.size, operation); !result)
                return result;
        }
        return {};
    }

    auto RenderingDeviceDriver::commandResolveImage(
        CommandBuffer* commandBuffer,
        Image* source,
        ImageLayout sourceLayout,
        uint32_t sourceLayer,
        uint32_t sourceMipmap,
        Image* destination,
        ImageLayout destinationLayout,
        uint32_t destinationLayer,
        uint32_t destinationMipmap
    ) -> std::expected<void, CommandError> {
        constexpr std::string_view operation = "commandResolveImage";
        if (auto result = checkRecording(commandBuffer, operation, QueueFamilyBits::Graphics, RenderingScope::Outside);
            !result)
            return result;
        if (source == nullptr || destination == nullptr)
            return invalidArgument(operation, "source or destination image is null");
        if (!isTransferLayout(sourceLayout, true) || !isTransferLayout(destinationLayout, false))
            return invalidArgument(operation, "image layout does not support the requested resolve direction");
        if (source->format.samples == ImageSamples::One || destination->format.samples != ImageSamples::One ||
            source->format.format != destination->format.format ||
            getImageAspects(source->format.format) != ImageAspectFlags{ImageAspectBits::Color})
            return invalidArgument(
                operation,
                "resolve requires matching color formats, a multisampled source, and a single-sampled destination");
        if (sourceMipmap >= 32 || sourceMipmap >= source->format.mipmapCount || destinationMipmap >= 32)
            return invalidArgument(operation, "resolve mip level is outside the supported range");
        const glm::uvec3 extent{
            std::max(1u, source->format.width >> sourceMipmap),
            std::max(1u, source->format.height >> sourceMipmap), std::max(1u, source->format.depth >> sourceMipmap)
        };
        if (auto result = checkImageRegion(source, {ImageAspectBits::Color, sourceMipmap, sourceLayer, 1}, {}, extent,
                                           operation); !result)
            return result;
        return checkImageRegion(destination, {ImageAspectBits::Color, destinationMipmap, destinationLayer, 1}, {},
                                extent, operation);
    }

    auto RenderingDeviceDriver::commandClearColorImage(
        CommandBuffer* commandBuffer,
        Image* image,
        ImageLayout imageLayout,
        const glm::vec4& color,
        const ImageSubresourceRange& subresource
    ) -> std::expected<void, CommandError> {
        constexpr std::string_view operation = "commandClearColorImage";
        (void)color;
        if (auto result = checkRecording(commandBuffer, operation, QueueFamilyBits::Graphics | QueueFamilyBits::Compute,
                                         RenderingScope::Outside); !result)
            return result;
        if (!isTransferLayout(imageLayout, false))
            return invalidArgument(operation, "image layout does not support clearing");
        if (auto result = checkImageRange(image, subresource, operation); !result)
            return result;
        if (subresource.aspect != ImageAspectFlags{ImageAspectBits::Color})
            return invalidArgument(operation, "color clear requires only the Color aspect");
        return {};
    }

    auto RenderingDeviceDriver::commandCopyBufferToImage(
        CommandBuffer* commandBuffer,
        Buffer* buffer,
        Image* image,
        ImageLayout layout,
        const std::vector<BufferImageCopyRegion>& regions
    ) -> std::expected<void, CommandError> {
        constexpr std::string_view operation = "commandCopyBufferToImage";
        if (auto result = checkRecording(commandBuffer, operation, QueueFamilyBits::Transfer, RenderingScope::Outside);
            !result)
            return result;
        if (buffer == nullptr || image == nullptr)
            return invalidArgument(operation, "buffer or image is null");
        if (!isTransferLayout(layout, false))
            return invalidArgument(operation, "image layout does not support the requested copy direction");
        if (regions.empty() || regions.size() > std::numeric_limits<uint32_t>::max())
            return invalidArgument(operation, "copy region count must be nonzero and representable as uint32_t");
        for (const auto& region : regions) {
            if (region.bufferOffset >= buffer->getSize())
                return invalidArgument(operation, "copy buffer offset is outside the buffer");
            if (auto result = checkImageRegion(image, region.imageSubresourceLayers, region.imageOffset,
                                               region.imageRegionSize, operation); !result)
                return result;
        }
        return {};
    }

    auto RenderingDeviceDriver::commandCopyImageToBuffer(
        CommandBuffer* commandBuffer,
        Image* image,
        ImageLayout layout,
        Buffer* buffer,
        const std::vector<BufferImageCopyRegion>& regions
    ) -> std::expected<void, CommandError> {
        constexpr std::string_view operation = "commandCopyImageToBuffer";
        if (auto result = checkRecording(commandBuffer, operation, QueueFamilyBits::Transfer, RenderingScope::Outside);
            !result)
            return result;
        if (buffer == nullptr || image == nullptr)
            return invalidArgument(operation, "buffer or image is null");
        if (!isTransferLayout(layout, true))
            return invalidArgument(operation, "image layout does not support the requested copy direction");
        if (regions.empty() || regions.size() > std::numeric_limits<uint32_t>::max())
            return invalidArgument(operation, "copy region count must be nonzero and representable as uint32_t");
        for (const auto& region : regions) {
            if (region.bufferOffset >= buffer->getSize())
                return invalidArgument(operation, "copy buffer offset is outside the buffer");
            if (auto result = checkImageRegion(image, region.imageSubresourceLayers, region.imageOffset,
                                               region.imageRegionSize, operation); !result)
                return result;
        }
        return {};
    }

    auto RenderingDeviceDriver::commandBeginLabel(
        CommandBuffer* commandBuffer,
        const std::string& label,
        const glm::vec4& color
    ) -> std::expected<void, CommandError> {
        constexpr std::string_view operation = "commandBeginLabel";
        (void)label;
        (void)color;
        return checkRecording(commandBuffer, operation);
    }

    auto RenderingDeviceDriver::commandEndLabel(
        CommandBuffer* commandBuffer
    ) -> std::expected<void, CommandError> {
        constexpr std::string_view operation = "commandEndLabel";
        return checkRecording(commandBuffer, operation);
    }

    auto RenderingDeviceDriver::resetCommandPool(CommandPool* pool)
        -> std::expected<void, CommandError> {
        if (pool == nullptr)
            return invalidArgument("resetCommandPool", "command pool is null");
        for (const auto& state : pool->commandSubmissions)
            if (state->load() == CommandBuffer::State::Pending)
                return commandError(CommandErrorCode::InvalidState, "resetCommandPool",
                                    "a native submission using the pool is still pending");
        for (const auto* commandBuffer : pool->commandBuffers)
            if (commandBuffer->getState() == CommandBuffer::State::Pending)
                return commandError(CommandErrorCode::InvalidState, "resetCommandPool",
                                    "a command buffer allocated from the pool is still pending");
        return {};
    }

    auto RenderingDeviceDriver::executeCommandQueueAndPresent(
        CommandQueue* commandQueue,
        const std::vector<Semaphore*>& waitSemaphores,
        const std::vector<CommandBuffer*>& commandBuffers,
        const std::vector<Semaphore*>& signalSemaphores,
        Fence* fence,
        const std::vector<Swapchain*>& swapchains
    ) -> std::expected<void, CommandError> {
        constexpr std::string_view operation = "executeCommandQueueAndPresent";
        (void)fence;
        if (commandQueue == nullptr)
            return invalidArgument(operation, "command queue is null");
        if (commandBuffers.size() > std::numeric_limits<uint32_t>::max() ||
            waitSemaphores.size() > std::numeric_limits<uint32_t>::max() ||
            signalSemaphores.size() > std::numeric_limits<uint32_t>::max())
            return invalidArgument(operation, "submission array count exceeds uint32_t");
        for (size_t i = 0; i < commandBuffers.size(); ++i) {
            auto* commandBuffer = commandBuffers[i];
            if (commandBuffer == nullptr)
                return invalidArgument(operation, "command buffer is null");
            if (commandBuffer->getState() != CommandBuffer::State::Executable)
                return commandError(CommandErrorCode::InvalidState, operation,
                                    "every submitted command buffer must be Executable");
            if (commandBuffer->pool == nullptr || commandBuffer->pool->type != CommandBufferType::Primary)
                return invalidArgument(
                    operation, "only primary command buffers from a live pool can be submitted directly");
            if (std::find(commandBuffers.begin(), commandBuffers.begin() + i, commandBuffer) != commandBuffers.begin() +
                i)
                return invalidArgument(operation, "the same command buffer occurs more than once in the submission");
        }
        for (auto* semaphore : waitSemaphores)
            if (semaphore == nullptr)
                return invalidArgument(operation, "wait semaphore is null");
        for (auto* semaphore : signalSemaphores)
            if (semaphore == nullptr)
                return invalidArgument(operation, "signal semaphore is null");
        for (auto* swapchain : swapchains)
            if (swapchain == nullptr)
                return invalidArgument(operation, "swapchain is null");
        return {};
    }
}
