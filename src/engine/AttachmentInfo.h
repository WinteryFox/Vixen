#pragma once

namespace Vixen {
    enum class LoadAction;

    enum class StoreAction;

    struct AttachmentInfo {
        LoadAction loadAction;

        StoreAction storeAction;

        VkImageLayout layout;

        VkImageView loadStoreTarget;

        VkImageView resolveTarget;

        glm::vec4 clearColor;

        float clearDepth;

        uint32_t clearStencil;
    };
}
