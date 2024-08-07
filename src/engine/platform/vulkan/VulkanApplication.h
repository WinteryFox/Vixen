#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <Volk/volk.h>

#include "core/Application.h"
#include "rendering/Renderer.h"
#include "shader/VulkanShaderProgram.h"

namespace Vixen {
    class Renderer;
    class VulkanPipeline;
    class VulkanSwapchain;
    class VulkanDevice;
    class Instance;
    class VulkanWindow;

    class VulkanApplication : public Application {
        struct UniformBufferObject {
            glm::mat4 view;

            glm::mat4 projection;
        };

        const std::vector<const char *> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        std::shared_ptr<VulkanWindow> window;

        std::shared_ptr<Instance> instance;

        VkSurfaceKHR surface;

        std::shared_ptr<VulkanDevice> device;

        std::shared_ptr<VulkanSwapchain> swapchain;

        std::shared_ptr<VulkanPipeline> pipeline;

        std::unique_ptr<Renderer> renderer;

        VulkanShaderProgram pbrOpaqueShader;

    public:
        VulkanApplication(const std::string &appTitle, glm::vec3 appVersion);

        VulkanApplication(const VulkanApplication &) = delete;

        VulkanApplication &operator=(const VulkanApplication &) = delete;

        [[nodiscard]] bool isRunning() const;

        void update();

        void render();

        [[nodiscard]] std::shared_ptr<VulkanWindow> getWindow() const;

        [[nodiscard]] std::shared_ptr<Instance> getInstance() const;

        [[nodiscard]] VkSurfaceKHR getSurface() const;

        [[nodiscard]] std::shared_ptr<VulkanDevice> getDevice() const;

        [[nodiscard]] std::shared_ptr<VulkanSwapchain> getSwapchain() const;
    };
}
