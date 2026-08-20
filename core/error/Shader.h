#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "shader/ShaderStage.h"

namespace Vixen {
    enum class ShaderReflectionErrorCode {
        None,

        NullOutputShader,
        NoShaderStages,
        DuplicateShaderStage,
        IncompatibleShaderStages,
        MissingRequiredShaderStage,
        InvalidShaderStage,

        EmptySpirv,
        InvalidSpirvSize,
        InvalidSpirvMagic,
        InvalidSpirv,
        SpirvReflectionFailed,

        NoEntryPoint,
        MultipleEntryPoints,
        InvalidEntryPointName,
        EntryPointStageMismatch,

        MissingDescriptorSet,
        MissingDescriptorBinding,

        RuntimeDescriptorArrayUnsupported,
        ZeroDescriptorCount,
        DescriptorCountOverflow,

        DuplicateDescriptorBinding,
        DescriptorTypeConflict,
        DescriptorCountConflict,
        DescriptorSizeConflict,

        MultiplePushConstantBlocks,
        EmptyPushConstantBlock,
        PushConstantSizeOverflow,
        PushConstantAlignmentInvalid,

        UnsupportedResourceType,

        DescriptorSetLimitExceeded,
        DescriptorTypeLimitExceeded,
        PushConstantLimitExceeded
    };

    struct ShaderReflectionError {
        ShaderReflectionErrorCode type = ShaderReflectionErrorCode::None;

        std::optional<ShaderStageBits> stages;

        std::string resourceName;

        std::optional<uint32_t> set;
        std::optional<uint32_t> binding;

        std::optional<uint64_t> expected;
        std::optional<uint64_t> actual;

        std::string detail;
    };
}
