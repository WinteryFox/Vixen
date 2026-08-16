#pragma once

#include <memory>
#include <string>
#include <glm/vec3.hpp>

#include "RenderingDriver.h"

namespace Vixen {
    class DisplayServer;

    class Application final {
        std::unique_ptr<DisplayServer> displayServer;

        std::string applicationTitle;

        glm::vec3 applicationVersion;

        std::string workingDirectory;

    public:
        explicit Application(
            RenderingDriver renderingDriver,
            const std::string& applicationTitle,
            glm::vec3 applicationVersion,
            std::string workingDirectory = ".\\"
        );

        ~Application();

        void run() const;
    };
}
