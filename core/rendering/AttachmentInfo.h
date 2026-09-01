#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "ClearValue.h"
#include "LoadAction.h"
#include "StoreAction.h"
#include "glm/vec2.hpp"
#include "core/image/ImageLayout.h"

namespace Vixen {
    struct Image;

    struct AttachmentInfo {
        Image* image = nullptr;

        ImageLayout layout = ImageLayout::ColorAttachmentOptimal;

        LoadAction loadAction = LoadAction::DontCare;

        StoreAction storeAction = StoreAction::Store;

        Image* resolveImage = nullptr;

        ClearValue clearValue{};
    };

    struct RenderingInfo {
        glm::uvec2 extent;

        uint32_t layerCount;

        std::vector<AttachmentInfo> colorAttachments;

        std::optional<AttachmentInfo> depthStencilAttachment;
    };
}
