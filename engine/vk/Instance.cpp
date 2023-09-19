#include "Instance.h"

namespace Vixen::Vk {
    Instance::Instance(const std::string &appName, glm::vec3 appVersion,
                       const std::vector<const char *> &requiredExtensions)
            : instance(VK_NULL_HANDLE) {
        volkInitialize();

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = appName.c_str();
        appInfo.applicationVersion = VK_MAKE_API_VERSION(0, appVersion.x, appVersion.y, appVersion.z);
        appInfo.pEngineName = "Vixen";
        appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        std::vector<const char *> extensions(requiredExtensions);
        extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

        std::vector<const char *> layers{};
#ifdef DEBUG
        if (isLayerSupported("VK_LAYER_KHRONOS_validation")) {
            layers.emplace_back("VK_LAYER_KHRONOS_validation");
            extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        } else {
            spdlog::warn("Debug mode is enabled but Vulkan validation layers are not available, did you install them?");
        }
#endif

        VkInstanceCreateInfo instanceInfo{};
        instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceInfo.pApplicationInfo = &appInfo;
        instanceInfo.enabledExtensionCount = extensions.size();
        instanceInfo.ppEnabledExtensionNames = extensions.data();
        instanceInfo.enabledLayerCount = layers.size();
        instanceInfo.ppEnabledLayerNames = layers.data();
        instanceInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

        spdlog::info(
                "Creating new Vulkan instance for app \"{} ({})\" with extensions [{}] and layers [{}]",
                appName,
                getVersionString(appVersion),
                fmt::join(extensions, ", "),
                fmt::join(layers, ", ")
        );
        checkVulkanResult(
                vkCreateInstance(&instanceInfo, nullptr, &instance),
                "Failed to create Vulkan instance"
        );
        volkLoadInstance(instance);

#ifdef DEBUG
        VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
        debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugInfo.messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
        debugInfo.pfnUserCallback = vkDebugCallback;

        vkCreateDebugUtilsMessengerEXT(instance, &debugInfo, nullptr, &debugMessenger);
#endif
    }

    Instance::~Instance() {
        for (auto surface: surfaces)
            vkDestroySurfaceKHR(instance, surface, nullptr);

#ifdef DEBUG
        vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
#endif
        vkDestroyInstance(instance, nullptr);
    }

    std::vector<GraphicsCard> Instance::getGraphicsCards() const {
        uint32_t deviceCount;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        std::vector<VkPhysicalDevice> devices{deviceCount};
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        std::vector<GraphicsCard> gpus{};
        gpus.reserve(deviceCount);
        for (const auto &gpu: devices)
            gpus.emplace_back(gpu);

        return gpus;
    }

    GraphicsCard
    Instance::findOptimalGraphicsCard(VkSurfaceKHR surface, const std::vector<const char *> &extensions) const {
        const auto &gpus = getGraphicsCards();
        if (gpus.empty())
            throw std::runtime_error("No graphics cards found");

        spdlog::trace("Attempting to find optimal GPU");
        bool foundSuitableGpu = false;
        GraphicsCard optimalCard = gpus[0];
        for (const auto &gpu: gpus) {
            if (!gpu.supportsExtensions(extensions)) {
                spdlog::debug("Disqualified gpu \"{}\" for missing extensions", gpu.properties.deviceName);
                continue;
            }

            if (gpu.getPresentModes(surface).empty()) {
                spdlog::debug("Disqualified gpu \"{}\" for lack of present mode support", gpu.properties.deviceName);
                continue;
            }

            if (gpu.getSurfaceFormats(surface).empty()) {
                spdlog::debug("Disqualified gpu \"{}\" for lack of surface format support", gpu.properties.deviceName);
                continue;
            }

            optimalCard = gpu;
            foundSuitableGpu = true;
        }

        if (!foundSuitableGpu)
            error("Failed to find suitable graphics card");
        spdlog::trace("Optimal graphics card is \"{}\"", optimalCard.properties.deviceName);

        // TODO: Implement
        return optimalCard;
    }

    std::vector<VkExtensionProperties> Instance::getSupportedExtensions() {
        uint32_t count;
        vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);

        std::vector<VkExtensionProperties> extensions(count);
        vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data());

        return extensions;
    }

    bool Instance::isExtensionSupported(const std::string &extension) {
        const auto extensions = getSupportedExtensions();

        return std::find_if(
                extensions.begin(),
                extensions.end(),
                [extension](VkExtensionProperties props) { return extension == props.extensionName; }
        ) != std::end(extensions);
    }

    std::vector<VkLayerProperties> Instance::getSupportedLayers() {
        uint32_t count;
        vkEnumerateInstanceLayerProperties(&count, nullptr);

        std::vector<VkLayerProperties> layers(count);
        vkEnumerateInstanceLayerProperties(&count, layers.data());

        return layers;
    }

    bool Instance::isLayerSupported(const std::string &layer) {
        const auto layers = getSupportedLayers();

        return std::find_if(
                layers.begin(),
                layers.end(),
                [layer](VkLayerProperties props) { return layer == props.layerName; }
        ) != std::end(layers);
    }

    /**
     * Creates a new surface for the given window. Surfaces are owned by this instance, and will be deleted alongside.
     * @param window The window to create the surface for.
     * @return Returns a new surface for this window.
     */
    VkSurfaceKHR Instance::surfaceForWindow(const VkWindow &window) {
        auto surface = window.createSurface(instance);
        surfaces.push_back(surface);

        return surface;
    }
}
