#pragma once

#include <glm/glm.hpp>

#include "Vulkan.h"
#include "GraphicsCard.h"
#include "window/VulkanWindow.h"

namespace Vixen {
    class Instance {
        friend class VulkanDevice;

        friend class VkVixen;

        VkInstance instance;

        std::vector<VkSurfaceKHR> surfaces;

#ifdef DEBUG
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
#endif

    public:
        /**
         * Create a new Vulkan instance. This constructor automatically selects a GPU to use for this instance.
         * @param appName The name of the application running.
         * @param appVersion The version of the application running.
         * @param requiredExtensions The non-optional extensions this app uses.
         */
        Instance(const std::string &appName, glm::vec3 appVersion, const std::vector<const char *> &requiredExtensions);

        ~Instance();

        [[nodiscard]] std::vector<GraphicsCard> getGraphicsCards() const;

        [[nodiscard]] GraphicsCard
        findOptimalGraphicsCard(VkSurfaceKHR surface, const std::vector<const char *> &extensions) const;

        static std::vector<VkExtensionProperties> getSupportedExtensions();

        static bool isExtensionSupported(const std::string &extension);

        static std::vector<VkLayerProperties> getSupportedLayers();

        static bool isLayerSupported(const std::string &layer);

        [[nodiscard]] VkSurfaceKHR surfaceForWindow(const VulkanWindow &window);
    };
}
