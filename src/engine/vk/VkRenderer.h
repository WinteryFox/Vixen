#pragma once

#include "Device.h"
#include "VkMesh.h"
#include "VkPipeline.h"
#include "VkPipelineLayout.h"

namespace Vixen::Vk {
    struct FrameData;
    class Swapchain;

    class VkRenderer {
        std::shared_ptr<Device> device;

        DeletionQueue deletionQueue;

        std::shared_ptr<Swapchain> swapchain;

        std::unique_ptr<VkPipelineLayout> pipelineLayout;

        std::shared_ptr<VkPipeline> pipeline;

    public:
        VkRenderer(
            const std::shared_ptr<VkPipeline> &pipeline,
            const std::shared_ptr<Swapchain> &swapchain
        );

        VkRenderer(const VkRenderer &) = delete;

        VkRenderer &operator=(const VkRenderer &) = delete;

        ~VkRenderer();

        void render(const std::vector<VkMesh> &meshes);

    private:
        void prepare(
            const FrameData &frame,
            const std::vector<VkMesh> &meshes
        ) const;

        void cleanup();
    };
}
