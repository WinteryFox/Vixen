#include "VkShaderProgram.h"

namespace Vixen::Vk {
    VkShaderProgram::VkShaderProgram(const std::vector<std::shared_ptr<VkShaderModule>> &modules)
            : ShaderProgram(modules) {}
}
