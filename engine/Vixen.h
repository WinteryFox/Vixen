#pragma once

#include <utility>

#include "Renderer.h"
#include "Window.h"

namespace Vixen::Engine {
    class Vixen {
    protected:
        std::shared_ptr<Renderer> renderer;

    public:
        const std::string appTitle;

        const glm::vec3 appVersion;

        Vixen(std::string appTitle, glm::vec3 appVersion, std::shared_ptr<Renderer> renderer)
                : appTitle(std::move(appTitle)), appVersion(appVersion), renderer(std::move(renderer)) {}
    };
}
