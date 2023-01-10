#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <fmt/color.h>
#include "Macro.h"
#include "GraphicsCard.h"
#include "Surface.h"

namespace Vixen::Engine::Vk {
    class Instance {
        friend class Surface;

        GraphicsCard gpu;

        VkInstance instance;

        VkDevice device;

        VkSurfaceKHR surface;

        VkQueue graphicsQueue;

        VkQueue presentQueue;

#ifdef DEBUG
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
#endif

        static VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback(
                VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                [[maybe_unused]] void *pUserData
        );

    public:
        /**
         * Create a new Vulkan instance. This constructor automatically selects a GPU to use for this instance.
         * @param appName The name of the application running.
         * @param appVersion The version of the application running.
         * @param requiredExtensions The non-optional extensions this app uses.
         */
        Instance(const std::string &appName, glm::vec3 appVersion, const std::vector<const char *> &requiredExtensions);

        /**
         * Create a new Vulkan instance using a specific GPU.
         * @param gpu The GPU to use for this instance.
         * @param appName The name of the application running.
         * @param appVersion The version of the application running.
         * @param requiredExtensions The non-optional extensions this app uses.
         */
        Instance(VkPhysicalDevice gpu, const std::string &appName, glm::vec3 appVersion,
                 const std::vector<const char *> &requiredExtensions);

        /**
         * Create a new Vulkan instance using a specific GPU.
         * @param gpu The GPU to use for this instance.
         * @param appName The name of the application running.
         * @param appVersion The version of the application running.
         * @param requiredExtensions The non-optional extensions this app uses.
         */
        Instance(GraphicsCard gpu, const std::string &appName, glm::vec3 appVersion,
                 const std::vector<const char *> &requiredExtensions);

        ~Instance();

        [[nodiscard]] std::vector<GraphicsCard> getGraphicsCards() const;

        [[nodiscard]] static GraphicsCard getGraphicsCardProperties(VkPhysicalDevice gpu);

        [[nodiscard]] GraphicsCard findOptimalGraphicsCard() const;

        static std::vector<VkExtensionProperties> getSupportedExtensions();

        static bool isExtensionSupported(const std::string &extension);

        static std::vector<VkLayerProperties> getSupportedLayers();

        static bool isLayerSupported(const std::string &layer);

        [[nodiscard]] VkQueue getQueueHandle(uint32_t queueFamilyIndex, uint32_t queueIndex = 0) const;

        [[nodiscard]] VkSurfaceKHR surfaceForWindow(const Window &window) const;
    };
}
