#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <Volk/volk.h>

#include "LoadAction.h"
#include "StoreAction.h"

namespace Vixen {
    class VulkanImageView;

    struct AttachmentInfo {
        LoadAction loadAction;

        StoreAction storeAction;

        VkImageView loadStoreTarget;

        VkImageView resolveTarget;

        glm::vec4 clearColor;

        float clearDepth;

        uint32_t clearStencil;
    };
}
