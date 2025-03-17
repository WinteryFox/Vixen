#include "Application.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <DisplayServer.h>
#include <utility>

#include "RenderingDevice.h"
#include "error/CantCreateError.h"
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

        this->displayServer = std::make_shared<DisplayServer>(
            applicationTitle,
            applicationVersion,
            renderingDriver,
            WindowMode::Windowed,
            VSyncMode::Disabled,
            WindowFlags::Resizable,
            glm::ivec2{1920, 1080}
        );
    }

    Application::~Application() {
    }

    void Application::run() const {
        const auto mainWindow = displayServer->getMainWindow();

        while (!displayServer->shouldClose(mainWindow)) {
            displayServer->update(mainWindow);

            renderingDevice->swapBuffers(true);
        }
    }

    std::shared_ptr<DisplayServer> Application::getDisplayServer() const {
        return displayServer;
    }

    RenderingContextDriver *Application::getRenderingContext() const {
        return renderingContext;
    }

    RenderingDevice *Application::getRenderingDevice() const {
        return renderingDevice;
    }
}
