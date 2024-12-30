#pragma once

#include <volk.h>

#include "core/Bitmask.h"

namespace Vixen {
    enum class CommandBufferType {
        Primary,
        Secondary
    };

    static_assert(ENUM_MEMBERS_EQUAL(CommandBufferType::Primary, VK_COMMAND_BUFFER_LEVEL_PRIMARY));
    static_assert(ENUM_MEMBERS_EQUAL(CommandBufferType::Secondary, VK_COMMAND_BUFFER_LEVEL_SECONDARY));
}
