#pragma once

namespace Vixen {
    enum class ShaderUniformType {
        Sampler,
        SampledImage,
        CombinedImageSampler,
        StorageImage,

        UniformBuffer,
        StorageBuffer,

        InputAttachment
    };
}
