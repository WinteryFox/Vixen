#pragma once

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include "../ShaderModule.h"
#include "Vulkan.h"
#include "Device.h"

#ifdef DEBUG

#include <glslang/SPIRV/disassemble.h>

#endif

namespace Vixen::Engine {
    class VkShaderModule : ShaderModule {
        ::VkShaderModule module;

        std::shared_ptr<Device> device;

    public:
        VkShaderModule(const std::shared_ptr<Device> &device, Stage stage, const std::string &source,
                       const std::string &entrypoint = "main");

        ~VkShaderModule();
    };
}
