#pragma once

#include "../ShaderProgram.h"
#include "GlShaderModule.h"

namespace Vixen::Gl {
    class GlShaderProgram : ShaderProgram<GlShaderModule> {
        unsigned int program;

    public:
        explicit GlShaderProgram(const std::shared_ptr<GlShaderModule> &vertex,
                                 const std::shared_ptr<GlShaderModule> &fragment);

        GlShaderProgram(const GlShaderProgram &) = delete;

        GlShaderProgram &operator=(const GlShaderProgram &) = delete;

        ~GlShaderProgram();

        void bind() const;

        static void unbind();
    };
}
