#pragma once

namespace Vixen {
    enum class ImageLayout {
        Undefined,
        General,
        StorageOptimal,
        ColorAttachmentOptimal,
        DepthStencilAttachmentOptimal,
        DepthStencilReadOnlyOptimal,
        ShaderReadOnlyOptimal,
        CopySourceOptimal,
        CopyDestinationOptimal,
        ResolveSourceOptimal,
        ResolveDestinationOptimal
    };
}
