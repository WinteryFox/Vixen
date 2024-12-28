#pragma once

#include <memory>

namespace Vixen {
    enum class MaterialPass;
    class VulkanDescriptorSet;
    class VulkanPipeline;
    class Image2D;

    struct Material {
        std::shared_ptr<VulkanPipeline> pipeline;
        std::shared_ptr<Image2D> image;
        std::shared_ptr<VulkanDescriptorSet> descriptorSet;
        MaterialPass pass;
    };
}
