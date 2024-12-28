#include "Application.h"

#include <DisplayServer.h>

namespace Vixen {
    Application::Application(
        DisplayServer::RenderingDriver renderingDriver,
        const std::string &applicationTitle,
        const glm::vec3 applicationVersion,
        const std::string &workingDirectory
    ) : applicationTitle(applicationTitle),
        applicationVersion(applicationVersion),
        workingDirectory(workingDirectory) {
        this->displayServer = std::make_shared<DisplayServer>(
            renderingDriver,
            DisplayServer::WindowMode::Maximized,
            DisplayServer::VSyncMode::Disabled,
            DisplayServer::WindowFlags::WINDOW_FLAGS_RESIZABLE,
            glm::ivec2{1920, 1080}
        );
    }

    Application::~Application() {
    }

    void Application::run() {
        while (!displayServer->shouldClose()) {
            displayServer->update();
        }
    }
}
