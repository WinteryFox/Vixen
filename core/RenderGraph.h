#pragma once

#include <vector>

#include "RenderPass.h"

namespace Vixen {
    class RenderGraph final {
        std::vector<RenderPass> renderPasses;

    public:
        ~RenderGraph();

        void addRenderPass(RenderPass renderPass);
    };
}
