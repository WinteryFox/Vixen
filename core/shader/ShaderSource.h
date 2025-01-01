#pragma once

#include <vector>

#include "ShaderLanguage.h"

namespace Vixen {
    struct ShaderSource {
        ShaderLanguage language;
        std::vector<std::byte> source;
    };
}
