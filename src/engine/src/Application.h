#pragma once

#include <string>
#include <utility>
#include <glm/vec3.hpp>

namespace Vixen {
    class Application {
        const std::string appTitle;

        const glm::vec3 appVersion;

    public:
        Application(std::string appTitle, const glm::vec3 appVersion)
            : appTitle(std::move(appTitle)), appVersion(appVersion) {}
    };
}
