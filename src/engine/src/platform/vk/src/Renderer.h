#pragma once

#include "VulkanDevice.h"
#include "VulkanMesh.h"
#include "pipeline/VulkanPipeline.h"
#include "pipeline/VulkanPipelineLayout.h"

namespace Vixen {
    class VulkanSwapchain;
    struct FrameData;

    class Renderer {
        std::shared_ptr<VulkanDevice> device;

        DeletionQueue deletionQueue;

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
