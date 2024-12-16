#pragma once

#include "core/Context.h"

namespace Vixen {
    class VulkanContext final : public Context {
    public:
        VulkanContext();

        ~VulkanContext() override;
    };
}
