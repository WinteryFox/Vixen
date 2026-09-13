#pragma once

#include <cstdint>
#include <memory>

namespace Vixen {
    class DescriptorPoolLifetime;

    class DescriptorSet {
        friend class RenderingDeviceDriver;

        std::weak_ptr<DescriptorPoolLifetime> lifetime;
        uint32_t allocationIndex = 0;
        uint64_t generation = 0;

        DescriptorSet(
            const std::shared_ptr<DescriptorPoolLifetime>& lifetime,
            uint32_t allocationIndex,
            uint64_t generation
        ) noexcept;

    public:
        DescriptorSet() = default;

        [[nodiscard]] bool isValid() const noexcept;
    };
}
