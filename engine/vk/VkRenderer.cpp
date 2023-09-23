#include "VkRenderer.h"

namespace Vixen::Vk {
    VkRenderer::VkRenderer(
            const std::shared_ptr<Vk::Device> &device,
            const Swapchain &swapchain,
            std::unique_ptr<Vk::VkPipeline> &pipeline
    ) : device(device),
        swapchain(swapchain),
        pipeline(std::move(pipeline)),
        pipelineLayout(std::make_unique<VkPipelineLayout>(device, pipeline->getProgram())),
        renderCommandPool(std::make_shared<VkCommandPool>(device, VkCommandPool::Usage::GRAPHICS)),
        renderCommandBuffers(VkCommandBuffer::allocateCommandBuffers(
                renderCommandPool,
                VkCommandBuffer::Level::PRIMARY,
                swapchain.getImageCount()
        )) {
        
    }

    VkRenderer::~VkRenderer() {
    }
}
