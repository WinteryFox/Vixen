#include "VulkanRenderingContextDriver.h"

#include <map>
#include <ranges>
#include <Vulkan.h>
#include <bits/ranges_algo.h>
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
            }
            else {
                error<CantCreateError>("Failed to get Vulkan API version.");
            }
        }
        else {
            spdlog::info("vkEnumerateInstanceVersion not available, assuming Vulkan 1.0");
            instanceApiVersion = VK_API_VERSION_1_0;
        }
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
                enabledInstanceExtensions.push_back(strdup(extensionName));
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
            .apiVersion = instanceApiVersion == VK_API_VERSION_1_0 ? VK_API_VERSION_1_0 : VK_API_VERSION_1_3
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
            .ppEnabledExtensionNames = enabledInstanceExtensions.data()
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
        uint32_t physicalDeviceCount;
        if (vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr) != VK_SUCCESS)
            error<CantCreateError>("Failed to enumerate physical devices.");
        physicalDevices.resize(physicalDeviceCount);
        deviceQueueFamilyProperties.resize(physicalDeviceCount);
        if (vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data()) != VK_SUCCESS)
            error<CantCreateError>("Failed to enumerate physical devices.");

        driverDevices.reserve(physicalDeviceCount);
        for (uint32_t i = 0; i < physicalDevices.size(); i++) {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physicalDevices[i], &properties);

            driverDevices.push_back(
                {
                    .name = properties.deviceName
                }
            );

            uint32_t queueFamilyPropertiesCount;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamilyPropertiesCount, nullptr);

            if (queueFamilyPropertiesCount > 0) {
                deviceQueueFamilyProperties[i].resize(queueFamilyPropertiesCount);
                vkGetPhysicalDeviceQueueFamilyProperties(
                    physicalDevices[i],
                    &queueFamilyPropertiesCount,
                    deviceQueueFamilyProperties[i].data()
                );
            }
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
        return driverDevices;
    }

    bool VulkanRenderingContextDriver::deviceSupportsPresent(
        const uint32_t deviceIndex,
        Surface* surface
    ) {
        DEBUG_ASSERT(deviceIndex < physicalDevices.size());
        DEBUG_ASSERT(surface != nullptr);
        DEBUG_ASSERT(dynamic_cast<VulkanSurface *>(surface)->surface != nullptr);

        const auto& vkSurface = dynamic_cast<VulkanSurface*>(surface);

        const auto physicalDevice = physicalDevices[deviceIndex];
        const auto& queueFamilies = deviceQueueFamilyProperties[deviceIndex];
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
        DEBUG_ASSERT(deviceIndex < deviceQueueFamilyProperties.size());

        return deviceQueueFamilyProperties[deviceIndex].size();
    }

    VkQueueFamilyProperties VulkanRenderingContextDriver::getQueueFamilyProperties(
        const uint32_t deviceIndex,
        const uint32_t queueFamilyIndex
    ) const {
        DEBUG_ASSERT(deviceIndex < deviceQueueFamilyProperties.size());
        DEBUG_ASSERT(queueFamilyIndex < deviceQueueFamilyProperties[deviceIndex].size());

        return deviceQueueFamilyProperties[deviceIndex][queueFamilyIndex];
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

        return physicalDevices[deviceIndex];
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

    void VulkanRenderingContextDriver::destroySurface(
        Surface* surface
    ) {
        const auto vkSurface = dynamic_cast<VulkanSurface*>(surface);
        vkDestroySurfaceKHR(instance, vkSurface->surface, nullptr);
        delete vkSurface;
    }
}
