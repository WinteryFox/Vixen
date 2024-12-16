#ifdef _WIN32

#include <Windows.h>

#endif

#include <format>
#include <spdlog/spdlog.h>

#include "core/Application.h"

int main() {
#ifdef _WIN32
    system(std::format("chcp {}", CP_UTF8).c_str());
#endif

#ifdef DEBUG
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("%Y-%m-%d %T.%e %^%7l%$ %P --- [%t] %1v");
#endif

    try {
        auto application = Vixen::Application({
            .renderingApi = Vixen::Vulkan,
            .applicationTitle = "Vixen Engine",
            .applicationVersion = {1, 0, 0}
        });

        application.run();
    } catch (const std::runtime_error &e) {
        spdlog::error(e.what());
        throw;
    }

    return EXIT_SUCCESS;
}
