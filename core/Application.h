#pragma once

#include "RenderingContextDriver.h"
#include "DisplayServer.h"

namespace Vixen {
    class Application final {
        std::shared_ptr<DisplayServer> displayServer;

        std::string applicationTitle;

        glm::vec3 applicationVersion;

        std::string workingDirectory;

    public:
        explicit Application(
            RenderingDriver renderingDriver,
            const std::string &applicationTitle,
            glm::vec3 applicationVersion,
            std::string workingDirectory = ".\\"
        );

        ~Application();

        void run() const;

        [[nodiscard]] std::shared_ptr<DisplayServer> getDisplayServer() const;

        RenderingContextDriver *getRenderingContext() const;

        RenderingDevice *getRenderingDevice() const;
    };
}
