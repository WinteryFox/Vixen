#include "RenderingContextDriver.h"

namespace Vixen {
    Surface* RenderingContextDriver::getSurfaceFromWindow(Window* window) const {
        if (const auto& pair = surfaces.find(window); pair != surfaces.end())
            return pair->second;

        return nullptr;
    }

    auto RenderingContextDriver::createWindow(Window* window) -> std::expected<void, Error> {
        const auto surface = createSurface(window);
        if (!surface)
            return std::unexpected(Error::InitializationFailed);

        surfaces[window] = surface.value();
        return {};
    }

    void RenderingContextDriver::setWindowSize(Window* window, uint32_t width, uint32_t height) {
        if (auto surface = getSurfaceFromWindow(window); surface != nullptr)
            setSurfaceSize(surface, width, height);
    }

    void RenderingContextDriver::setWindowVSyncMode(Window* window, VSyncMode vsyncMode) {
        if (auto surface = getSurfaceFromWindow(window); surface != nullptr)
            setSurfaceVSyncMode(surface, vsyncMode);
    }

    void RenderingContextDriver::destroyWindow(Window* window) {
        if (const auto& surface = getSurfaceFromWindow(window); surface != nullptr)
            destroySurface(surface);
        surfaces.erase(window);
    }
}
