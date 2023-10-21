#include "VkShaderProgram.h"

namespace Vixen::Vk {
    VkShaderProgram::VkShaderProgram(const std::map<ShaderModule::Stage, std::shared_ptr<VkShaderModule>> &modules)
            : ShaderProgram(modules) {}
}
