#include "VkRenderer.h"

namespace Vixen::Vk {
    VkRenderer::VkRenderer(const std::shared_ptr<Vk::Device> &device,
                           std::unique_ptr<Vk::VkPipeline> &&pipeline)
            : device(device),
              pipeline(std::move(pipeline)),
              pipelineLayout(std::make_unique<VkPipelineLayout>(device, pipeline->getProgram())) {

    }

    VkRenderer::~VkRenderer() {
    }
}
