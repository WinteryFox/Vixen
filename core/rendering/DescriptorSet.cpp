#include "DescriptorSet.h"

#include "DescriptorPool.h"

namespace Vixen {
    DescriptorSet::DescriptorSet(
        const std::shared_ptr<DescriptorPoolLifetime>& lifetime,
        const uint32_t allocationIndex,
        const uint64_t generation
    ) noexcept : lifetime(lifetime),
                 allocationIndex(allocationIndex),
                 generation(generation) {}

    bool DescriptorSet::isValid() const noexcept {
        const auto state = lifetime.lock();
        return state &&
            state->owner != nullptr &&
            state->generation == generation;
    }
}
