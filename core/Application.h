#pragma once

#include "DisplayServer.h"

namespace Vixen {
    class Application {
        std::shared_ptr<DisplayServer> displayServer;

        std::string applicationTitle;

        glm::vec3 applicationVersion;

        std::string workingDirectory;

    public:
        explicit Application(
            DisplayServer::RenderingDriver renderingDriver,
            const std::string &applicationTitle,
            glm::vec3 applicationVersion,
            const std::string &workingDirectory = ".\\"
        );

        virtual ~Application();

        virtual void run();
    };
}
