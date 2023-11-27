#pragma once

#include <utility>
#include <glm/vec3.hpp>

#include "Renderer.h"
#include "Window.h"

namespace Vixen {
    class Vixen {
    protected:
        std::shared_ptr<Renderer> renderer;

    public:
        const std::string appTitle;

        const glm::vec3 appVersion;

        Vixen(std::string appTitle, glm::vec3 appVersion)
                : appTitle(std::move(appTitle)), appVersion(appVersion) {}
    };
}
