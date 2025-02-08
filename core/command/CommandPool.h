#pragma once

#include <cstdint>

#include "CommandBufferType.h"

namespace Vixen {
    enum class CommandBufferType;

    struct CommandPool {
        uint32_t queueFamily{};

        CommandBufferType type = CommandBufferType::Primary;

        virtual ~CommandPool() = default;
    };
}
