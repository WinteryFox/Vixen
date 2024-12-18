#pragma once

#include "../../core/RenderingDevice.h"

namespace Vixen {
    class VulkanRenderingDevice : public RenderingDevice {
    public:
        VulkanRenderingDevice();

        ~VulkanRenderingDevice();
    };
}
