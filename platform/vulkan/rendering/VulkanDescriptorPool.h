#pragma once

#include <vector>
#include <volk.h>

#include "rendering/DescriptorPool.h"

namespace Vixen {
    struct VulkanDescriptorPoolBlock {
        VkDescriptorPool pool = VK_NULL_HANDLE;
        bool exhausted = false;
    };

    class VulkanDescriptorPool final : public DescriptorPool {
        friend class VulkanRenderingDeviceDriver;

        uint32_t currentBlockIndex = 0;
        std::vector<VulkanDescriptorPoolBlock> blocks;
        std::vector<VkDescriptorSet> nativeSets;

    public:
        VulkanDescriptorPool() = default;
    };
}
