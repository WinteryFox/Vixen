#pragma once

#include <GL/glew.h>
#include "../ShaderProgram.h"
#include "ShaderModule.h"

namespace Vixen::Engine::Gl {
    class ShaderProgram : Vixen::Engine::ShaderProgram<Gl::ShaderModule> {
        unsigned int program;

    public:
        explicit ShaderProgram(const std::vector<std::shared_ptr<Gl::ShaderModule>> &modules);

        ShaderProgram(const ShaderProgram &) = delete;

        ShaderProgram &operator=(const ShaderProgram &) = delete;

        ~ShaderProgram();

        void bind() const;

        void unbind() const;
    };
}
