#pragma once

#include <cstdint>
#include <string>
#include <glm/glm.hpp>

#include "shader/ShaderStage.h"

namespace Vixen {
    struct Image;
    enum class PipelineStageFlags : unsigned;

    class RenderPass {
        ShaderStage stage = ShaderStage::Vertex;

        std::vector<std::string> attachments{};
        std::vector<std::string> colorInputs{};
        std::vector<std::tuple<std::string, Image*>> colorOutputs{};

        std::tuple<std::string, Image*> depthStencilOutput{};
        std::string depthStencilInput{};

        glm::vec4 clearColor = {};
        float depth = 1.0f;
        uint32_t stencil = 0;

    public:
        virtual ~RenderPass() = default;

        void addAttachment(std::string attachment);
    };
}
