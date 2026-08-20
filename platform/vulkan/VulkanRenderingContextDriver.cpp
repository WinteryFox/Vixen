#include "VulkanRenderingContextDriver.h"

#include <algorithm>
#include <map>
#include <ranges>
#include "Vulkan.h"
#include <GLFW/glfw3.h>

#include "VulkanRenderingDeviceDriver.h"
#include "VulkanSurface.h"
#include "core/Window.h"
#include "core/error/CantCreateError.h"
#include "core/error/Macros.h"

namespace Vixen {
    void VulkanRenderingContextDriver::initializeVulkanVersion() {
        if (const auto func = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(vkGetInstanceProcAddr(
            nullptr,
            "vkEnumerateInstanceVersion"
        )); func != nullptr) {
            uint32_t api_version;

            if (const VkResult res = func(&api_version); res == VK_SUCCESS) {
                instanceApiVersion = api_version;
            } else {
                error<CantCreateError>("Failed to get Vulkan API version.");
            }
        } else {
            spdlog::info("vkEnumerateInstanceVersion not available, assuming Vulkan 1.0");
            instanceApiVersion = VK_API_VERSION_1_0;
        }

        if (instanceApiVersion < VK_API_VERSION_1_3)
            error<CantCreateError>("Vulkan loader/runtime does not support Vulkan 1.3.");
    }

    void VulkanRenderingContextDriver::initializeInstanceExtensions() {
        enabledInstanceExtensions.clear();

        std::map<std::string, bool> requestedExtensions{};
        {
            uint32_t count;
            const char** extensions = glfwGetRequiredInstanceExtensions(&count);
            for (uint32_t i = 0; i < count; i++)
                requestedExtensions[std::string(extensions[i])] = true;
        }

        #ifdef DEBUG_ENABLED
        requestedExtensions[VK_EXT_DEBUG_REPORT_EXTENSION_NAME] = false;
        requestedExtensions[VK_EXT_DEBUG_UTILS_EXTENSION_NAME] = false;
        #endif

        requestedExtensions[VK_KHR_SURFACE_EXTENSION_NAME] = true;

        #if defined(MACOS_ENABLED) || defined(IOS_ENABLED)
        requestedExtensions[VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME] = true;
        #endif

        requestedExtensions[VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME] = false;

        uint32_t extensionCount = 0;
        if (vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr) != VK_SUCCESS)
            error<CantCreateError>("Call to vkEnumerateInstanceExtensions failed.");
        std::vector<VkExtensionProperties> availableExtensions{extensionCount};
        if (vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data()) != VK_SUCCESS)
            error<CantCreateError>("Call to vkEnumerateInstanceExtensions failed.");


        spdlog::trace(
            "Found the following Vulkan instance extensions.\n{}",
            std::ranges::fold_left(
                availableExtensions |
                std::views::transform(
                    [](
                    const auto& extension
                ) {
                        return "    - " + std::string(extension.extensionName);
                    }
                ),
                std::string{},
                [](
                const auto& a,
                const auto& b
            ) {
                    return a.empty() ? std::move(b) : std::move(a) + "\n" + std::move(b);
                }
            )
        );
        for (uint32_t i = 0; i < extensionCount; i++) {
            if (const auto& extensionName = availableExtensions[i].extensionName;
                requestedExtensions.contains(extensionName))
                enabledInstanceExtensions.emplace_back(extensionName);
        }

        for (const auto& [extensionName, required] : requestedExtensions) {
            if (std::ranges::find(enabledInstanceExtensions.begin(), enabledInstanceExtensions.end(), extensionName) ==
                enabledInstanceExtensions.end()) {
                if (required)
                    error<CantCreateError>("Required extension \"" + extensionName + "\" was not found");

                spdlog::debug("Optional extension {} was not found.", extensionName);
            }
        }
    }

    void VulkanRenderingContextDriver::initializeInstance(
        const std::string& applicationName,
        const glm::ivec3& applicationVersion
    ) {
        VkApplicationInfo applicationInfo{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = applicationName.c_str(),
            .applicationVersion = VK_MAKE_VERSION(applicationVersion.x, applicationVersion.y, applicationVersion.z),
            .pEngineName = ENGINE_NAME,
            .engineVersion = VK_MAKE_VERSION(ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR, ENGINE_VERSION_PATCH),
            .apiVersion = VK_API_VERSION_1_3
        };

        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers{layerCount};
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        std::vector<const char*> enabledLayerNames{};

        #ifdef DEBUG_ENABLED
        for (const auto& layer : availableLayers)
            if (layer.layerName == std::string("VK_LAYER_KHRONOS_validation"))
                enabledLayerNames.push_back("VK_LAYER_KHRONOS_validation");
        #endif

        std::vector<const char*> enabledInstanceExtensionsStr{enabledInstanceExtensions.size()};
        for (auto i = 0; i < enabledInstanceExtensions.size(); i++) {
            enabledInstanceExtensionsStr[i] = enabledInstanceExtensions[i].c_str();
        }

        VkInstanceCreateInfo instanceInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            #if defined(MACOS_ENABLED) || defined(IOS_ENABLED)
            .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
            #else
            .flags = 0,
            #endif
            .pApplicationInfo = &applicationInfo,
            .enabledLayerCount = static_cast<uint32_t>(enabledLayerNames.size()),
            .ppEnabledLayerNames = enabledLayerNames.data(),
            .enabledExtensionCount = static_cast<uint32_t>(enabledInstanceExtensions.size()),
            .ppEnabledExtensionNames = enabledInstanceExtensionsStr.data()
        };

        #ifdef DEBUG_ENABLED
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = {};
        if (std::ranges::find(
            enabledInstanceExtensions.begin(),
            enabledInstanceExtensions.end(),
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME
        ) != enabledInstanceExtensions.end()) {
            debugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugMessengerInfo.pNext = nullptr;
            debugMessengerInfo.flags = 0;
            debugMessengerInfo.messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugMessengerInfo.pfnUserCallback = vkDebugCallback;
            debugMessengerInfo.pUserData = this;
            instanceInfo.pNext = &debugMessengerInfo;
        }
        #endif

        const auto res = vkCreateInstance(&instanceInfo, nullptr, &instance);
        if (res == VK_ERROR_INCOMPATIBLE_DRIVER)
            error<CantCreateError>(
                "Cannot find a compatible Vulkan installable client driver (ICD).\n"
                "Updating your graphics drivers may resolve this issue.\n"
                "vkCreateInstance failed."
            );
        if (res == VK_ERROR_EXTENSION_NOT_PRESENT)
            error<CantCreateError>(
                "Cannot find a specified extension library.\n"
                "Make sure your layers path is set appropriately.\n"
                "Updating your graphics drivers may resolve this issue.\n"
                "vkCreateInstance failed."
            );
        if (res != VK_SUCCESS)
            error<CantCreateError>(
                "Failed to create Vulkan instance.\n"
                "Do you have a Vulkan compatible graphics driver installed?\n"
                "Updating your graphics drivers may resolve this issue.\n"
                "vkCreateInstance failed."
            );

        volkLoadInstance(instance);
    }

    void VulkanRenderingContextDriver::initializeDevices() {
        uint32_t physicalDeviceCount = 0;
        if (vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr) != VK_SUCCESS)
            error<CantCreateError>("Failed to enumerate physical devices.");

        std::vector<VkPhysicalDevice> availableDevices(physicalDeviceCount);
        if (vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, availableDevices.data()) != VK_SUCCESS)
            error<CantCreateError>("Failed to enumerate physical devices.");

        physicalDevices.clear();
        physicalDevices.reserve(availableDevices.size());

        for (const VkPhysicalDevice handle : availableDevices) {
            PhysicalDeviceRecord record{
                .handle = handle
            };

            vkGetPhysicalDeviceProperties(handle, &record.properties);
            vkGetPhysicalDeviceMemoryProperties(handle, &record.memoryProperties);

            if (record.properties.apiVersion < VK_API_VERSION_1_3) {
                spdlog::debug("Ignoring device '{}': Vulkan 1.3 is not supported.", record.properties.deviceName);
                continue;
            }

            DeviceFeatureSupport features{};
            features.core.pNext = &features.vulkan12;
            features.vulkan12.pNext = &features.vulkan13;
            vkGetPhysicalDeviceFeatures2(handle, &features.core);

            if (features.vulkan12.timelineSemaphore != VK_TRUE ||
                features.vulkan13.dynamicRendering != VK_TRUE ||
                features.vulkan13.synchronization2 != VK_TRUE ||
                features.core.features.imageCubeArray != VK_TRUE ||
                features.core.features.independentBlend != VK_TRUE) {
                spdlog::debug("Ignoring device '{}': required Vulkan features are unavailable.",
                              record.properties.deviceName);
                continue;
            }

            uint32_t extensionCount = 0;
            if (vkEnumerateDeviceExtensionProperties(handle, nullptr, &extensionCount, nullptr) != VK_SUCCESS) {
                spdlog::debug("Ignoring device '{}': extensions could not be enumerated.",
                              record.properties.deviceName);
                continue;
            }

            std::vector<VkExtensionProperties> extensions(extensionCount);
            if (vkEnumerateDeviceExtensionProperties(handle, nullptr, &extensionCount, extensions.data()) !=
                VK_SUCCESS) {
                spdlog::debug("Ignoring device '{}': extensions could not be enumerated.",
                              record.properties.deviceName);
                continue;
            }

            const bool supportsSwapchain = std::ranges::any_of(
                extensions,
                [](const VkExtensionProperties& extension) {
                    return std::string_view(extension.extensionName) == VK_KHR_SWAPCHAIN_EXTENSION_NAME;
                }
            );
            if (!supportsSwapchain) {
                spdlog::debug("Ignoring device '{}': VK_KHR_swapchain is unavailable.",
                              record.properties.deviceName);
                continue;
            }

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(handle, &queueFamilyCount, nullptr);
            record.queueFamilies.resize(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(
                handle,
                &queueFamilyCount,
                record.queueFamilies.data()
            );

            const bool hasGraphicsComputeQueue = std::ranges::any_of(
                record.queueFamilies,
                [](const VkQueueFamilyProperties& queueFamily) {
                    constexpr VkQueueFlags required = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
                    return queueFamily.queueCount > 0 && (queueFamily.queueFlags & required) == required;
                }
            );
            const bool hasTransferQueue = std::ranges::any_of(
                record.queueFamilies,
                [](const VkQueueFamilyProperties& queueFamily) {
                    return queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;
                }
            );
            if (!hasGraphicsComputeQueue || !hasTransferQueue) {
                spdlog::debug("Ignoring device '{}': required queue families are unavailable.",
                              record.properties.deviceName);
                continue;
            }

            uint64_t deviceLocalMemory = 0;
            for (uint32_t heap = 0; heap < record.memoryProperties.memoryHeapCount; ++heap) {
                if ((record.memoryProperties.memoryHeaps[heap].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0)
                    deviceLocalMemory += record.memoryProperties.memoryHeaps[heap].size;
            }

            const bool hasDedicatedComputeQueue = std::ranges::any_of(
                record.queueFamilies,
                [](const VkQueueFamilyProperties& queueFamily) {
                    return queueFamily.queueCount > 0 &&
                        (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0 &&
                        (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0;
                }
            );
            const bool hasDedicatedTransferQueue = std::ranges::any_of(
                record.queueFamilies,
                [](const VkQueueFamilyProperties& queueFamily) {
                    return queueFamily.queueCount > 0 &&
                        (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0 &&
                        (queueFamily.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == 0;
                }
            );

            DriverDeviceType deviceType = DriverDeviceType::Other;
            switch (record.properties.deviceType) {
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    deviceType = DriverDeviceType::Integrated;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    deviceType = DriverDeviceType::Discrete;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    deviceType = DriverDeviceType::Virtual;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    deviceType = DriverDeviceType::Cpu;
                    break;
                default:
                    break;
            }

            record.description = {
                .name = record.properties.deviceName,
                .type = deviceType,
                .deviceLocalMemory = deviceLocalMemory,
                .hasDedicatedComputeQueue = hasDedicatedComputeQueue,
                .hasDedicatedTransferQueue = hasDedicatedTransferQueue
            };

            physicalDevices.push_back(std::move(record));
        }
    }

    VulkanRenderingContextDriver::VulkanRenderingContextDriver(
        const std::string& applicationName,
        const glm::ivec3& applicationVersion
    ) : RenderingContextDriver(),
        instanceApiVersion(VK_API_VERSION_1_0),
        instance(VK_NULL_HANDLE) {
        if (glfwVulkanSupported() != GLFW_TRUE)
            error<CantCreateError>(
                "This device does not report Vulkan support.\n"
                "Updating your graphics drivers may resolve this issue.\n"
                "glfwVulkanSupported did not return GLFW_TRUE."
            );

        if (volkInitialize() != VK_SUCCESS)
            error<CantCreateError>(
                "Failed to initialize Volk.\n"
                "volkInitialize did not return VK_SUCCESS."
            );

        initializeVulkanVersion();

        initializeInstanceExtensions();

        initializeInstance(applicationName, applicationVersion);

        initializeDevices();
    }

    VulkanRenderingContextDriver::~VulkanRenderingContextDriver() {
        vkDestroyInstance(instance, nullptr);
    }

    std::vector<DriverDevice> VulkanRenderingContextDriver::getDevices() {
        return physicalDevices |
            std::views::transform([](const PhysicalDeviceRecord& device) { return device.description; }) |
            std::ranges::to<std::vector>();
    }

    bool VulkanRenderingContextDriver::deviceSupportsPresent(
        const uint32_t deviceIndex,
        Surface* surface
    ) {
        DEBUG_ASSERT(deviceIndex < physicalDevices.size());
        DEBUG_ASSERT(surface != nullptr);
        DEBUG_ASSERT(dynamic_cast<VulkanSurface *>(surface)->surface != nullptr);

        const auto& vkSurface = dynamic_cast<VulkanSurface*>(surface);

        const auto physicalDevice = physicalDevices[deviceIndex].handle;
        const auto& queueFamilies = physicalDevices[deviceIndex].queueFamilies;
        for (uint32_t i = 0; i < queueFamilies.size(); i++) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                VkBool32 presentSupport = VK_FALSE;
                if (vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, vkSurface->surface, &presentSupport) !=
                    VK_SUCCESS)
                    continue;

                if (presentSupport)
                    return true;
            }
        }

        return false;
    }

    uint32_t VulkanRenderingContextDriver::getQueueFamilyCount(
        const uint32_t deviceIndex
    ) const {
        DEBUG_ASSERT(deviceIndex < physicalDevices.size());

        return physicalDevices[deviceIndex].queueFamilies.size();
    }

    VkQueueFamilyProperties VulkanRenderingContextDriver::getQueueFamilyProperties(
        const uint32_t deviceIndex,
        const uint32_t queueFamilyIndex
    ) const {
        DEBUG_ASSERT(deviceIndex < physicalDevices.size());
        DEBUG_ASSERT(queueFamilyIndex < physicalDevices[deviceIndex].queueFamilies.size());

        return physicalDevices[deviceIndex].queueFamilies[queueFamilyIndex];
    }

    bool VulkanRenderingContextDriver::queueFamilySupportsPresent(
        VkPhysicalDevice physicalDevice,
        const uint32_t queueFamilyIndex,
        const VulkanSurface* surface
    ) {
        VkBool32 supportsPresent = VK_FALSE;
        const auto result = vkGetPhysicalDeviceSurfaceSupportKHR(
            physicalDevice,
            queueFamilyIndex,
            surface->surface,
            &supportsPresent
        );

        return result == VK_SUCCESS && supportsPresent;
    }

    VkPhysicalDevice VulkanRenderingContextDriver::getPhysicalDevice(
        const uint32_t deviceIndex
    ) const {
        DEBUG_ASSERT(deviceIndex < physicalDevices.size());

        return physicalDevices[deviceIndex].handle;
    }

    RenderingDeviceDriver* VulkanRenderingContextDriver::createRenderingDeviceDriver(
        const uint32_t deviceIndex,
        const uint32_t frameCount
    ) {
        return new VulkanRenderingDeviceDriver(this, deviceIndex, frameCount);
    }

    void VulkanRenderingContextDriver::destroyRenderingDeviceDriver(RenderingDeviceDriver* renderingDeviceDriver) {
        const auto vkRenderingDeviceDriver = dynamic_cast<VulkanRenderingDeviceDriver*>(renderingDeviceDriver);
        delete vkRenderingDeviceDriver;
    }

    VkInstance VulkanRenderingContextDriver::getInstance() const {
        return instance;
    }

    uint32_t VulkanRenderingContextDriver::getInstanceApiVersion() const {
        return instanceApiVersion;
    }

    auto VulkanRenderingContextDriver::createSurface(
        Window* window
    ) -> std::expected<Surface*, Error> {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        if (glfwCreateWindowSurface(instance, window->window, nullptr, &surface) != VK_SUCCESS)
            return std::unexpected(Error::InitializationFailed);

        const auto o = new VulkanSurface();
        o->surface = surface;

        return o;
    }

    bool VulkanRenderingContextDriver::getSurfaceNeedsResize(Surface* surface) {
        return surface->isResizeRequired;
    }

    void VulkanRenderingContextDriver::setSurfaceNeedsResize(Surface* surface, bool needsResize) {
        surface->isResizeRequired = needsResize;
    }

    void VulkanRenderingContextDriver::setSurfaceSize(Surface* surface, uint32_t width, uint32_t height) {
        surface->resolution = {
            width,
            height
        };
        surface->isResizeRequired = true;
    }

    void VulkanRenderingContextDriver::setSurfaceVSyncMode(Surface* surface, VSyncMode vsyncMode) {
        surface->vsyncMode = vsyncMode;
        surface->isResizeRequired = true;
    }

    void VulkanRenderingContextDriver::setSurfaceWindowMode(Surface* surface, WindowMode mode) {
        surface->windowMode = mode;
    }

    void VulkanRenderingContextDriver::destroySurface(
        Surface* surface
    ) {
        const auto vkSurface = dynamic_cast<VulkanSurface*>(surface);
        vkDestroySurfaceKHR(instance, vkSurface->surface, nullptr);
        delete vkSurface;
    }
}
