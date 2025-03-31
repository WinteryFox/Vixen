#include "Application.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <DisplayServer.h>
#include <utility>

#include "RenderingDevice.h"
#include "error/Macros.h"
#include "platform/vulkan/VulkanRenderingContextDriver.h"

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

        displayServer = std::make_unique<DisplayServer>(
            applicationTitle,
            applicationVersion,
            renderingDriver,
            WindowMode::Windowed,
            VSyncMode::Disabled,
            WindowFlags::Resizable,
            glm::uvec2{1920, 1080}
        );
    }

    Application::~Application() = default;

    void Application::run() const {
        const auto mainWindow = displayServer->getMainWindow();

        while (!displayServer->shouldClose(mainWindow)) {
            displayServer->update(mainWindow);
        }
    }
}
