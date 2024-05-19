#pragma once

#include <LoadAction.h>
#include <StoreAction.h>

#include "src/image/VulkanImageView.h"

namespace Vixen {
    struct AttachmentInfo {
        LoadAction loadAction;

        StoreAction storeAction;

        std::shared_ptr<VulkanImageView> loadStoreTarget;

        std::shared_ptr<VulkanImageView> resolveTarget;

        glm::vec4 clearColor;

        float clearDepth;

        uint32_t clearStencil;
    };
}
