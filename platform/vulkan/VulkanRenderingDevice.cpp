#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "VulkanRenderingDevice.h"

#include <vma/vk_mem_alloc.h>

#include "FrameData.h"
#include "VulkanRenderingContext.h"
#include "buffer/VulkanBuffer.h"

namespace Vixen {
    void VulkanRenderingDevice::initializeExtensions() {
        std::map<std::string, bool> requestedExtensions;

        requestedExtensions[VK_KHR_SWAPCHAIN_EXTENSION_NAME] = true;
        requestedExtensions[VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME] = true;
        requestedExtensions[VK_KHR_MAINTENANCE_2_EXTENSION_NAME] = false;

#ifdef DEBUG_ENABLED
        requestedExtensions[VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME] = false;
        requestedExtensions[VK_EXT_DEVICE_FAULT_EXTENSION_NAME] = false;
        requestedExtensions[VK_EXT_DEBUG_MARKER_EXTENSION_NAME] = false;
#endif

        uint32_t extensionCount = 0;
        ASSERT_THROW(
            vkEnumerateDeviceExtensionProperties(physicalDevice.device, nullptr, &extensionCount, nullptr) ==
            VK_SUCCESS,
            CantCreateError,
            "Call to vkEnumerateDeviceExtensionProperties failed."
        );
        std::vector<VkExtensionProperties> availableExtensions{extensionCount};
        ASSERT_THROW(
            vkEnumerateDeviceExtensionProperties(physicalDevice.device, nullptr, &extensionCount, availableExtensions.
                data()) == VK_SUCCESS,
            CantCreateError,
            "Call to vkEnumerateDeviceExtensionProperties failed."
        );

        for (uint32_t i = 0; i < extensionCount; i++) {
            const auto &extensionName = availableExtensions[i].extensionName;
            spdlog::trace("VULKAN: Found device extension {}.", extensionName);
            if (requestedExtensions.contains(extensionName))
                enabledExtensionNames.push_back(strdup(extensionName));
        }

        for (const auto &[extensionName, required]: requestedExtensions) {
            if (std::ranges::find(enabledExtensionNames.begin(), enabledExtensionNames.end(), extensionName) ==
                enabledExtensionNames.end()) {
                ASSERT_THROW(!required, CantCreateError, "Required extension \"" + extensionName + "\" was not found");

                spdlog::debug("Optional extension {} was not found.", extensionName);
            }
        }
    }

    void VulkanRenderingDevice::checkFeatures() const {
        ASSERT_THROW(physicalDevice.features.imageCubeArray, CantCreateError, "Device lacks image cube array feature.");
        ASSERT_THROW(physicalDevice.features.independentBlend, CantCreateError,
                     "Device lacks independent blend feature.");
    }

    void VulkanRenderingDevice::checkCapabilities() {
        if (std::ranges::find(enabledExtensionNames, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME) != enabledExtensionNames.
            end())
            enabledFeatures.dynamicRendering = true;
        if (std::ranges::find(enabledExtensionNames, VK_EXT_DEVICE_FAULT_EXTENSION_NAME) != enabledExtensionNames.end())
            enabledFeatures.deviceFault = true;
    }

    void VulkanRenderingDevice::initializeDevice() {
        const auto queueFamilies = physicalDevice.getQueueFamilyWithFlags(
            VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{queueFamilies.size()};
        std::vector queuePriorities{0.0f};
        for (uint32_t i = 0; i < queueFamilies.size(); i++) {
            queueCreateInfos[i] = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .queueFamilyIndex = queueFamilies[i].index,
                .queueCount = 1,
                .pQueuePriorities = queuePriorities.data()
            };
        }

        VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
            .pNext = nullptr,
            .dynamicRendering = VK_TRUE
        };

        VkPhysicalDeviceFaultFeaturesEXT faultFeatures{};
        if (enabledFeatures.deviceFault) {
            faultFeatures = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT,
                .pNext = nullptr,
                .deviceFault = VK_FALSE,
                .deviceFaultVendorBinary = VK_FALSE
            };
            dynamicRenderingFeatures.pNext = &faultFeatures;
            spdlog::trace("Device fault feature is enabled.");
        }

        const VkDeviceCreateInfo deviceInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &dynamicRenderingFeatures,
            .flags = 0,
            .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
            .pQueueCreateInfos = queueCreateInfos.data(),
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = static_cast<uint32_t>(enabledExtensionNames.size()),
            .ppEnabledExtensionNames = enabledExtensionNames.data(),
            .pEnabledFeatures = nullptr // TODO
        };

        ASSERT_THROW(vkCreateDevice(physicalDevice.device, &deviceInfo, nullptr, &device) == VK_SUCCESS,
                     CantCreateError,
                     "Failed to create VkDevice");

        volkLoadDevice(device);

        const VmaVulkanFunctions vulkanFunctions{
            .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
            .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
            .vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties,
            .vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties,
            .vkAllocateMemory = vkAllocateMemory,
            .vkFreeMemory = vkFreeMemory,
            .vkMapMemory = vkMapMemory,
            .vkUnmapMemory = vkUnmapMemory,
            .vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges,
            .vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges,
            .vkBindBufferMemory = vkBindBufferMemory,
            .vkBindImageMemory = vkBindImageMemory,
            .vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements,
            .vkGetImageMemoryRequirements = vkGetImageMemoryRequirements,
            .vkCreateBuffer = vkCreateBuffer,
            .vkDestroyBuffer = vkDestroyBuffer,
            .vkCreateImage = vkCreateImage,
            .vkDestroyImage = vkDestroyImage,
            .vkCmdCopyBuffer = vkCmdCopyBuffer,
            .vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2,
            .vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2,
            .vkBindBufferMemory2KHR = vkBindBufferMemory2,
            .vkBindImageMemory2KHR = vkBindImageMemory2,
            .vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2,
            .vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements,
            .vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements,
        };

        const VmaAllocatorCreateInfo allocatorInfo{
            .flags = 0,
            .physicalDevice = physicalDevice.device,
            .device = device,
            .preferredLargeHeapBlockSize = 0,
            .pAllocationCallbacks = nullptr,
            .pDeviceMemoryCallbacks = nullptr,
            .pHeapSizeLimit = nullptr,
            .pVulkanFunctions = &vulkanFunctions,
            .instance = renderingContext->getInstance(),
            .vulkanApiVersion = VK_API_VERSION_1_3,
            .pTypeExternalMemoryHandleTypes = nullptr
        };
        ASSERT_THROW(vmaCreateAllocator(&allocatorInfo, &allocator) == VK_SUCCESS,
                     CantCreateError,
                     "Call to vmaCreateAllocator failed.");
    }

    VulkanRenderingDevice::VulkanRenderingDevice(
        const std::shared_ptr<VulkanRenderingContext> &renderingContext,
        const uint32_t deviceIndex
    ) : enabledFeatures(),
        renderingContext(renderingContext),
        physicalDevice(renderingContext->getPhysicalDevice(deviceIndex)),
        device(VK_NULL_HANDLE),
        allocator(VK_NULL_HANDLE) {
        initializeExtensions();

        checkFeatures();

        checkCapabilities();

        initializeDevice();
    }

    VulkanRenderingDevice::~VulkanRenderingDevice() {
        vmaDestroyAllocator(allocator);

        vkDestroyDevice(device, nullptr);
    };

    Buffer VulkanRenderingDevice::createBuffer(Buffer::Usage usage, uint32_t count, uint32_t stride) {
        // TODO
    }

    Image VulkanRenderingDevice::createImage(const ImageFormat &format, const ImageView &view) {
        // TODO
    }

    GraphicsCard VulkanRenderingDevice::getPhysicalDevice() const {
        return physicalDevice;
    }

    VkDevice VulkanRenderingDevice::getDevice() const {
        return device;
    }

    VmaAllocator VulkanRenderingDevice::getAllocator() const {
        return allocator;
    }
}
