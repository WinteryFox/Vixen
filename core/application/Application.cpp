#include "Application.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include "core/display/DisplayServer.h"
#include <utility>

#include "core/rendering/RenderingDevice.h"
#include "core/error/Macros.h"
#include "platform/vulkan/context/VulkanRenderingContextDriver.h"
#include <spdlog/spdlog.h>

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

        spdlog::debug(
            "Starting {} {}.{}.{} with rendering driver {} (working directory: '{}')",
            applicationTitle,
            static_cast<int>(applicationVersion.x),
            static_cast<int>(applicationVersion.y),
            static_cast<int>(applicationVersion.z),
            static_cast<uint32_t>(renderingDriver),
            this->workingDirectory
        );

        displayServer = std::make_unique<DisplayServer>(
            applicationTitle,
            applicationVersion,
            renderingDriver,
            WindowMode::Windowed,
            VSyncMode::Disabled,
            WindowBits::Resizable,
            glm::uvec2{1920, 1080}
        );
    }

    Application::~Application() = default;

    void Application::run() const {
        const auto mainWindow = displayServer->getMainWindow();

        spdlog::debug("Entering the application event loop");

        while (!displayServer->shouldClose(mainWindow)) {
            displayServer->update(mainWindow);
        }

        spdlog::debug("Leaving the application event loop");
    }
}
