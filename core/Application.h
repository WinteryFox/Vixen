#pragma once

#include <string>
#include <glm/vec3.hpp>

#include "Context.h"
#include "RenderingApi.h"

namespace Vixen {
    class Window;

    class Application {
        const struct Config {
            RenderingApi renderingApi;
            std::string applicationTitle;
            glm::vec3 applicationVersion;
            std::string workingDirectory = ".\\";
        } config;

        std::shared_ptr<Context> context;

        std::shared_ptr<Window> window;

    public:
        explicit Application(const Config &config);

        virtual ~Application();

        virtual void run();
    };
}
