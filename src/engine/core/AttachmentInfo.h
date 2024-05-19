#pragma once

#include <memory>
#include <glm/glm.hpp>

#include "LoadAction.h"
#include "StoreAction.h"

namespace Vixen {
    class VulkanImageView;

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
