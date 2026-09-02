#include "PipelineLayout.h"

namespace Vixen {
    PipelineLayout::PipelineLayout(
        PipelineLayoutDescription description
    ) : description(std::move(description)) {}

    const PipelineLayoutDescription& PipelineLayout::getDescription() const noexcept {
        return description;
    }
}
