#pragma once

#include <GL/glew.h>
#include "../ShaderProgram.h"
#include "GlShaderModule.h"

namespace Vixen::Engine::Gl {
    class GlShaderProgram : ShaderProgram<GlShaderModule> {
        unsigned int program;

    public:
        explicit GlShaderProgram(const std::vector<std::shared_ptr<GlShaderModule>> &modules);

        GlShaderProgram(const GlShaderProgram &) = delete;

        GlShaderProgram &operator=(const GlShaderProgram &) = delete;

        ~GlShaderProgram();

        void bind() const;

        static void unbind() ;
    };
}
