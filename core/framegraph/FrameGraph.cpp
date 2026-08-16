#include "FrameGraph.h"

namespace Vixen {
    FrameGraph::FrameGraph(std::vector<RenderPass>&& renderPasses)
        : renderPasses(std::move(renderPasses)) {}
}
