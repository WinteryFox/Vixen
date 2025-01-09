#include "Application.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <DisplayServer.h>
#include <utility>

namespace Vixen {
    Application::Application(
        RenderingDriver renderingDriver,
        const std::string &applicationTitle,
        const glm::vec3 applicationVersion,
        std::string workingDirectory
    ) : applicationTitle(applicationTitle),
        applicationVersion(applicationVersion),
        workingDirectory(std::move(workingDirectory)) {
#ifdef _WIN32
        system(std::format("chcp {}", CP_UTF8).c_str());
#endif

        spdlog::set_pattern("%Y-%m-%d %T.%e %^%7l%$ %P --- [%t] %1v");
#ifdef DEBUG_ENABLED
        spdlog::set_level(spdlog::level::trace);
#endif

        this->displayServer = std::make_shared<DisplayServer>(
            applicationTitle,
            renderingDriver,
            WindowMode::Windowed,
            VSyncMode::Disabled,
            WindowFlags::Resizable,
            glm::ivec2{1920, 1080}
        );
    }

    Application::~Application() = default;

    void Application::run() const {
        while (!displayServer->shouldClose()) {
            displayServer->update();
        }
    }

    std::shared_ptr<DisplayServer> Application::getDisplayServer() const {
        return displayServer;
    }
}
