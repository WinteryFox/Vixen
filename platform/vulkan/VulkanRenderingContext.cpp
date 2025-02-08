#include "VulkanRenderingContext.h"

#include <map>
#include <Vulkan.h>
#include <GLFW/glfw3.h>

#include "VulkanRenderingDevice.h"
#include "core/Window.h"
#include "core/error/CantCreateError.h"
#include "core/error/Macros.h"

namespace Vixen {
    void VulkanRenderingContext::initializeVulkanVersion() {
        if (const auto func = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(vkGetInstanceProcAddr(
            nullptr, "vkEnumerateInstanceVersion")); func != nullptr) {
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
    }

    void VulkanRenderingContext::initializeInstanceExtensions() {
        enabledInstanceExtensions.clear();

        std::map<std::string, bool> requestedExtensions{}; {
            uint32_t count;
            const char **extensions = glfwGetRequiredInstanceExtensions(&count);
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

        for (uint32_t i = 0; i < extensionCount; i++) {
            const auto &extensionName = availableExtensions[i].extensionName;
            spdlog::trace("VULKAN: Found instance extension {}.", extensionName);
            if (requestedExtensions.contains(extensionName))
                enabledInstanceExtensions.push_back(strdup(extensionName));
        }

        for (const auto &[extensionName, required]: requestedExtensions) {
            if (std::ranges::find(enabledInstanceExtensions.begin(), enabledInstanceExtensions.end(), extensionName) ==
                enabledInstanceExtensions.end()) {
                if (required)
                    error<CantCreateError>("Required extension \"" + extensionName + "\" was not found");

                spdlog::debug("Optional extension {} was not found.", extensionName);
            }
        }
    }

    void VulkanRenderingContext::initializeInstance(const std::string &applicationName,
                                                    const glm::ivec3 &applicationVersion) {
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

        std::vector<const char *> enabledLayerNames{};

#ifdef DEBUG_ENABLED
        for (const auto &layer: availableLayers)
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
        if (std::ranges::find(enabledInstanceExtensions.begin(), enabledInstanceExtensions.end(),
                              VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != enabledInstanceExtensions.end()) {
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
            error<CantCreateError>("Cannot find a compatible Vulkan installable client driver (ICD).\n"
                "Updating your graphics drivers may resolve this issue.\n"
                "vkCreateInstance failed.");
        if (res == VK_ERROR_EXTENSION_NOT_PRESENT)
            error<CantCreateError>("Cannot find a specified extension library.\n"
                "Make sure your layers path is set appropriately.\n"
                "Updating your graphics drivers may resolve this issue.\n"
                "vkCreateInstance failed.");
        if (res != VK_SUCCESS)
            error<CantCreateError>("Failed to create Vulkan instance.\n"
                "Do you have a Vulkan compatible graphics driver installed?\n"
                "Updating your graphics drivers may resolve this issue.\n"
                "vkCreateInstance failed.");

        volkLoadInstance(instance);
    }

    void VulkanRenderingContext::initializeDevices() {
        uint32_t physicalDeviceCount;
        if (vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr) != VK_SUCCESS)
            error<CantCreateError>("Failed to enumerate physical devices.");
        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        if (vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data()) != VK_SUCCESS)
            error<CantCreateError>("Failed to enumerate physical devices.");

        for (const auto &device: physicalDevices)
            this->physicalDevices.emplace_back(device);
    }

    VulkanRenderingContext::VulkanRenderingContext(const std::string &applicationName,
                                                   const glm::ivec3 &applicationVersion)
        : RenderingContext(),
          instanceApiVersion(VK_API_VERSION_1_0),
          instance(VK_NULL_HANDLE) {
        if (glfwVulkanSupported() != GLFW_TRUE)
            error<CantCreateError>("This device does not report Vulkan support.\n"
                "Updating your graphics drivers may resolve this issue.\n"
                "glfwVulkanSupported did not return GLFW_TRUE.");

        if (volkInitialize() != VK_SUCCESS)
            error<CantCreateError>("Failed to initialize Volk.\n"
                "volkInitialize did not return VK_SUCCESS.");

        initializeVulkanVersion();

        initializeInstanceExtensions();

        initializeInstance(applicationName, applicationVersion);

        initializeDevices();
    }

    VulkanRenderingContext::~VulkanRenderingContext() {
        vkDestroyInstance(instance, nullptr);
    }

    RenderingDevice *VulkanRenderingContext::createDevice() {
        return new VulkanRenderingDevice(this, 0);
    }

    GraphicsCard VulkanRenderingContext::getPhysicalDevice(const uint32_t index) {
        return physicalDevices[index];
    }

    VkInstance VulkanRenderingContext::getInstance() const {
        return instance;
    }

    uint32_t VulkanRenderingContext::getInstanceApiVersion() const {
        return instanceApiVersion;
    }

    bool VulkanRenderingContext::supportsPresent(VkPhysicalDevice physicalDevice, const uint32_t queueFamilyIndex,
                                                 const VulkanSurface *surface) {
        VkBool32 support = VK_FALSE;
        if (vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface->surface, &support) !=
            VK_SUCCESS)
            error<CantCreateError>("Failed to query surface support");

        return support;
    }

    Surface *VulkanRenderingContext::createSurface(Window *window) {
        auto *surface = dynamic_cast<VulkanSurface *>(window->surface);
        if (glfwCreateWindowSurface(instance, window->window, nullptr, &surface->surface) != VK_SUCCESS)
            error<CantCreateError>("Failed to create window surface.");

        return surface;
    }

    void VulkanRenderingContext::destroySurface(Surface *surface) {
        if (const auto vkSurface = dynamic_cast<VulkanSurface *>(surface)) {
            vkDestroySurfaceKHR(instance, vkSurface->surface, nullptr);
            delete vkSurface;
        }
    }
}
