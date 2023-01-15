#include "Instance.h"

#include <utility>

namespace Vixen::Engine::Vk {
    Instance::Instance(const std::string &appName, glm::vec3 appVersion,
                       const std::vector<const char *> &requiredExtensions)
            : instance(VK_NULL_HANDLE), device(VK_NULL_HANDLE) {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = appName.c_str();
        appInfo.applicationVersion = VK_MAKE_VERSION(appVersion.x, appVersion.y, appVersion.z);
        appInfo.pEngineName = "Vixen";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        std::vector<const char *> extensions(requiredExtensions);
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        std::vector<const char *> layers{};
#ifdef DEBUG
        if (isLayerSupported("VK_LAYER_KHRONOS_validation")) {
            layers.emplace_back("VK_LAYER_KHRONOS_validation");
            extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            spdlog::debug("Enabling Vulkan validation layers");
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

        VK_CHECK(vkCreateInstance(&instanceInfo, nullptr, &instance), "Failed to create Vk instance")

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
                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugInfo.pfnUserCallback = vkDebugCallback;

        auto func = getInstanceProcAddress<PFN_vkCreateDebugUtilsMessengerEXT>(instance,
                                                                               "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr)
            func(instance, &debugInfo, nullptr, &debugMessenger);
#endif

        gpu = findOptimalGraphicsCard();

        // TODO: Device and queue creation
        /*std::vector<QueueFamily> queueFamilies;
        queueFamilies.push_back(gpu.getQueueFamilyWithFlags(VK_QUEUE_GRAPHICS_BIT)[0]); // TODO
        queueFamilies.push_back(gpu.getSurfaceSupportedQueues(surface)[0]); // TODO
        std::vector<VkDeviceQueueCreateInfo> queueInfos{2};
        for (const auto &index: queueFamilyIndices) {
            VkDeviceQueueCreateInfo queueInfo{};
            queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex = index;
            queueInfo.queueCount = 1;
            float queuePriority = 1.0f;
            queueInfo.pQueuePriorities = &queuePriority;
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.queueCreateInfoCount = queueInfos.size();
        deviceInfo.pQueueCreateInfos = queueInfos.data();
        deviceInfo.pEnabledFeatures = &deviceFeatures;

        VK_CHECK(vkCreateDevice(gpu, &deviceInfo, nullptr, &device), "Failed to create Vulkan device")*/
    }

    Instance::Instance(VkPhysicalDevice gpu, const std::string &appName, glm::vec3 appVersion,
                       const std::vector<const char *> &requiredExtensions)
            : Instance(appName, appVersion, requiredExtensions) {
        this->gpu = getGraphicsCardProperties(gpu);
    }

    Instance::Instance(GraphicsCard gpu, const std::string &appName, glm::vec3 appVersion,
                       const std::vector<const char *> &requiredExtensions)
            : Instance(appName, appVersion, requiredExtensions) {
        this->gpu = std::move(gpu);
    }

    Instance::~Instance() {
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyDevice(device, nullptr);
#ifdef DEBUG
        auto func = getInstanceProcAddress<PFN_vkDestroyDebugUtilsMessengerEXT>(instance,
                                                                                "vkDestroyDebugUtilsMessengerEXT");
        if (func)
            func(instance, debugMessenger, nullptr);
#endif
        vkDestroyInstance(instance, nullptr);
    }

    std::vector<GraphicsCard> Instance::getGraphicsCards() const {
        uint32_t deviceCount;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        std::vector<VkPhysicalDevice> devices{deviceCount};
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        std::vector<GraphicsCard> gpus{deviceCount};
        for (uint32_t i = 0; i < gpus.size(); i++) {
            gpus[i] = getGraphicsCardProperties(devices[i]);
        }

        return gpus;
    }

    GraphicsCard Instance::getGraphicsCardProperties(VkPhysicalDevice physicalDevice) {
        GraphicsCard card;

        card.device = physicalDevice;
        vkGetPhysicalDeviceProperties(physicalDevice, &card.properties);
        vkGetPhysicalDeviceFeatures(physicalDevice, &card.features);

        uint32_t queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

        card.queueFamilies.resize(queueFamilyCount);
        for (uint32_t x = 0; x < queueFamilyCount; x++) {
            card.queueFamilies[x] = QueueFamily{
                    .index = x,
                    .properties = queueFamilies[x]
            };
        }
        return card;
    }

    GraphicsCard Instance::findOptimalGraphicsCard() const {
        const auto &gpus = getGraphicsCards();
        if (gpus.empty())
            throw std::runtime_error("No graphics cards found");

        // TODO: Implement
        return gpus[0];
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

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantFunctionResult"

    VkBool32 VKAPI_CALL Instance::vkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                  VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                  const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                  [[maybe_unused]] void *pUserData) {
        spdlog::level::level_enum level;
        switch (messageSeverity) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                level = spdlog::level::trace;
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                level = spdlog::level::info;
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                level = spdlog::level::warn;
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                level = spdlog::level::err;
                break;
            default:
                spdlog::warn("Unknown level flag in vkDebugCallback");
                return VK_FALSE;
        }

        std::string source;
        switch (messageType) {
            case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
                source = "Performance";
                break;
            case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
                source = "Validation";
                break;
            case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
                source = "General";
                break;
            default:
                spdlog::warn("Unknown type flag in vkDebugCallback");
                return VK_FALSE;
        }

        spdlog::log(
                level,
                "[{}] {}",
                fmt::format(fmt::fg(fmt::terminal_color::magenta), source),
                pCallbackData->pMessage
        );
        return VK_FALSE;
    }

#pragma clang diagnostic pop

    VkQueue Instance::getQueueHandle(uint32_t queueFamilyIndex, uint32_t queueIndex) const {
        VkQueue queue = VK_NULL_HANDLE;
        vkGetDeviceQueue(device, queueFamilyIndex, queueIndex, &queue);
        if (queue == VK_NULL_HANDLE)
            spdlog::error("Failed to get queue handle for queue family {} and index {}", queueFamilyIndex, queueIndex);

        return queue;
    }

    VkSurfaceKHR Instance::surfaceForWindow(const VkWindow &window) const {
        return window.createSurface(instance);
    }
}
