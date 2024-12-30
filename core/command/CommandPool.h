#pragma once

#include <cstdint>

namespace Vixen {
    enum class CommandBufferType;

    struct CommandPool {
        uint32_t queueFamily;

        CommandBufferType type;
    };
}
