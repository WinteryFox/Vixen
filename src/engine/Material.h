#pragma once

#include <memory>
#include "ShaderProgram.h"

namespace Vixen {
    class Material {
    public:
        std::shared_ptr<ShaderProgram> shader;
    };
}
