#pragma once

#include <memory>

#include "VkShaderProgram.h"

namespace Vixen {
    struct Material {
        std::shared_ptr<Vk::VkShaderProgram> shader;
    };
}
