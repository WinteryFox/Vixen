#include "Application.h"

#include <Window.h>

#include "platform/vulkan/VulkanContext.h"

namespace Vixen {
    Application::Application(const Config &config)
        : config(config) {
        switch (this->config.renderingApi) {
            case Vulkan:
                this->context = std::make_shared<VulkanContext>();
                break;
            case DirectX12:
                throw std::runtime_error("DirectX12 not supported");
            default:
                throw std::runtime_error("Unknown rendering API");
        }

        this->window = std::make_shared<Window>(config.applicationTitle, 1280, 720, false);

        this->window->center();
        this->window->setVisible(true);
    }

    Application::~Application() {
    }

    void Application::run() {
        while (!window->shouldClose()) {
            window->update();
        }
    }
}
