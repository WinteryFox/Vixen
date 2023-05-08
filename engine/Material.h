#pragma once

#include <memory>
#include "ShaderProgram.h"

namespace Vixen::Engine {
    class Material {
    public:
        std::shared_ptr<ShaderProgram> shader;
    };
}
