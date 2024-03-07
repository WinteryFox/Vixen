#pragma once

#include "MaterialPass.h"
#include "MaterialPipeline.h"
#include "VkDescriptorSet.h"

namespace Vixen::Vk {
    struct Material {
        std::shared_ptr<VkPipeline> pipeline;
        std::shared_ptr<VkImage> image;
        std::shared_ptr<VkImageView> imageView;
        std::shared_ptr<VkDescriptorSet> descriptorSet;
        MaterialPass pass;
    };
}
