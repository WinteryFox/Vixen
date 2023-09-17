#include "VkShaderProgram.h"

namespace Vixen::Engine {
    VkShaderProgram::VkShaderProgram(const std::vector<std::shared_ptr<ShaderModule>> &modules)
            : ShaderProgram(modules) {}
}
