#pragma once

namespace Vixen {
    enum class MaterialPass;
    class VulkanDescriptorSet;
    class VulkanPipeline;

    struct Material {
        std::shared_ptr<VulkanPipeline> pipeline;
        std::shared_ptr<VulkanImage> image;
        std::shared_ptr<VulkanImageView> imageView;
        std::shared_ptr<VulkanDescriptorSet> descriptorSet;
        MaterialPass pass;
    };
}
