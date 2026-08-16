#include "FrameGraph.h"

namespace Vixen {
    FrameGraph::FrameGraph(std::vector<ResourceNode>&& resources, std::vector<RenderPass>&& renderPasses)
        : resources(std::move(resources)),
          renderPasses(std::move(renderPasses)) {}
}
