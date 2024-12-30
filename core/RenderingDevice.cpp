#include "RenderingDevice.h"

#ifdef DEBUG_ENABLED
#include <glslang/SPIRV/disassemble.h>
#endif

#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <spdlog/spdlog.h>

#include "error/CantCreateError.h"
#include "error/Macros.h"

namespace Vixen {
    std::vector<std::byte> RenderingDevice::compileSpirvFromSource(const std::string &name, ShaderStage stage,
                                                                   const std::string &source) {
        EShLanguage language;
        switch (stage) {
            case ShaderStage::Vertex:
                language = EShLangVertex;
                break;

            case ShaderStage::Fragment:
                language = EShLangFragment;
                break;

            case ShaderStage::TesselationControl:
                language = EShLangTessControl;
                break;

            case ShaderStage::TesselationEvaluation:
                language = EShLangTessEvaluation;
                break;

            case ShaderStage::Compute:
                language = EShLangCompute;
                break;

            case ShaderStage::Geometry:
                language = EShLangGeometry;
                break;

            default:
                ASSERT_THROW(false, CantCreateError, "Unknown shader stage");
        }

        glslang::InitializeProcess();

        glslang::TShader shader{language};
        auto src = source.data();
        auto length = static_cast<int>(source.size());
        auto n = name.c_str();
        shader.setStringsWithLengthsAndNames(&src, &length, &n, 1);
        shader.setEnvInput(glslang::EShSourceGlsl, language, glslang::EShClientVulkan, 160);
        shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
        shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);

        glslang::TProgram program;

        // TODO: Add actual includer
        glslang::TShader::ForbidIncluder includer;

        // TODO: Resource limit should probably be gotten from the GPU
        ASSERT_THROW(shader.parse(GetDefaultResources(), 160, false, EShMsgEnhanced), CantCreateError,
                     "Failed to parse SPIR-V");

        program.addShader(&shader);
        ASSERT_THROW(program.link(EShMsgEnhanced), CantCreateError, "Failed to link shader");

        glslang::SpvOptions options{
#ifdef DEBUG_ENABLED
            .generateDebugInfo = true,
            .stripDebugInfo = false,
            .disableOptimizer = true,
            .optimizeSize = false,
            .disassemble = true,
#else
            .generateDebugInfo = false,
            .stripDebugInfo = true,
            .disableOptimizer = false,
            .optimizeSize = true,
            .disassemble = false,
#endif
            .validate = true,
        };

        spv::SpvBuildLogger logger;
        std::vector<uint32_t> binary{};
        GlslangToSpv(*program.getIntermediate(language), binary, &logger, &options);

#ifdef DEBUG_ENABLED
        std::stringstream stream;
        spv::Disassemble(stream, binary);
        spdlog::debug("Passed in GLSL source string:\n{}\n\nDisassembled SPIR-V:\n{}",
                      std::string_view(source.begin(), source.end()), stream.str());
#endif
        glslang::FinalizeProcess();

        std::vector<std::byte> result{binary.size() * sizeof(uint32_t)};
        memcpy(result.data(), binary.data(), binary.size() * sizeof(uint32_t));

        return result;
    }
}
