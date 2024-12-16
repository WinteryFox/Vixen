#pragma once

#include <memory>
#include <vector>

namespace Vixen {
    class VulkanMesh;
    struct FrameData;
    class VulkanPipeline;
    class VulkanPipelineLayout;
    class VulkanSwapchain;
    class VulkanDevice;

    class Renderer {
        std::shared_ptr<VulkanDevice> device;

        std::shared_ptr<VulkanSwapchain> swapchain;

        std::unique_ptr<VulkanPipelineLayout> pipelineLayout;

        std::shared_ptr<VulkanPipeline> pipeline;

    public:
        Renderer(
            const std::shared_ptr<VulkanPipeline> &pipeline,
            const std::shared_ptr<VulkanSwapchain> &swapchain
        );

        Renderer(const Renderer &) = delete;

        Renderer &operator=(const Renderer &) = delete;

        ~Renderer();

        void render(const std::vector<VulkanMesh> &meshes);

    private:
        void prepare(
            const FrameData &frame,
            const std::vector<VulkanMesh> &meshes
        ) const;

        void cleanup();
    };
}
