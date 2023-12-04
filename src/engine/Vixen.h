#pragma once

#include <string>
#include <utility>
#include <glm/vec3.hpp>

namespace Vixen {
    class Vixen {
    public:
        const std::string appTitle;

        const glm::vec3 appVersion;

        Vixen(std::string appTitle, const glm::vec3 appVersion)
            : appTitle(std::move(appTitle)), appVersion(appVersion) {}
    };
}
