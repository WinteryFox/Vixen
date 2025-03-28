#include <fstream>

#include "core/Application.h"
#include "core/RenderingDevice.h"
#include "core/RenderingDeviceDriver.h"
#include "core/command/CommandBufferType.h"

int main() {
    try {
        const auto application = Vixen::Application(
            Vixen::RenderingDriver::Vulkan,
            "Vixen " ENGINE_VERSION,
            {ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR, ENGINE_VERSION_PATCH}
        );

        application.run();
    } catch (const std::runtime_error &e) {
        spdlog::error(e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
