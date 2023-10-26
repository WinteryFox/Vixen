#include "VkShaderProgram.h"

namespace Vixen::Vk {
    VkShaderProgram::VkShaderProgram(
            const std::shared_ptr<VkShaderModule> &vertex,
            const std::shared_ptr<VkShaderModule> &fragment
    ) : ShaderProgram(vertex, fragment) {}
}
