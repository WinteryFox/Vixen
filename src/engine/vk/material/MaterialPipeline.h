#pragma once

#include "VkPipeline.h"

namespace Vixen::Vk {
    struct MaterialPipeline {
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;
    };
}
