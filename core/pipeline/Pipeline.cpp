#include "Pipeline.h"

namespace Vixen {
    Pipeline::Pipeline(
        const PipelineLayout& layout
    ) : layout(layout) {}

    const PipelineLayout& Pipeline::getLayout() const noexcept {
        return layout;
    }
}
