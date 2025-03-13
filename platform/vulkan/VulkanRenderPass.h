#pragma once

#include <volk.h>

#include "core/RenderPass.h"

namespace Vixen {
    class VulkanRenderPass final : public RenderPass {
    public:
        VkRenderPass renderPass;
    };
}
