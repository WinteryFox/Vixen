#define VMA_IMPLEMENTATION

#include "VulkanRenderingDevice.h"

#include <vk_mem_alloc.h>
#include <Vulkan.h>
#include <spirv_reflect.hpp>

#include "VulkanRenderingContext.h"
#include "VulkanSwapchain.h"
#include "buffer/VulkanBuffer.h"
#include "command/VulkanCommandBuffer.h"
#include "command/VulkanCommandPool.h"
#include "command/VulkanCommandQueue.h"
#include "command/VulkanSemaphore.h"
#include "command/VulkanFence.h"
#include "core/error/CantCreateError.h"
#include "image/VulkanImage.h"
#include "image/VulkanSampler.h"
#include "shader/VulkanShader.h"

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
        const auto families = physicalDevice.getQueueFamilyWithFlags(
            VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{families.size()};
        static constexpr float queuePriorities[1] = {0.0f};
        for (uint32_t i = 0; i < queueCreateInfos.size(); i++) {
            queueCreateInfos[i] = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .queueFamilyIndex = families[i].index,
                .queueCount = std::min(families[i].properties.queueCount, static_cast<uint32_t>(1)),
                .pQueuePriorities = queuePriorities
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

        VkPhysicalDeviceFeatures deviceFeatures{
            .robustBufferAccess = VK_FALSE,
            .fullDrawIndexUint32 = VK_FALSE,
            .imageCubeArray = VK_TRUE,
            .independentBlend = VK_TRUE,
            .geometryShader = VK_FALSE,
            .tessellationShader = VK_FALSE,
            .sampleRateShading = VK_FALSE,
            .dualSrcBlend = VK_FALSE,
            .logicOp = VK_FALSE,
            .multiDrawIndirect = VK_FALSE,
            .drawIndirectFirstInstance = VK_FALSE,
            .depthClamp = VK_FALSE,
            .depthBiasClamp = VK_FALSE,
            .fillModeNonSolid = VK_FALSE,
            .depthBounds = VK_FALSE,
            .wideLines = VK_FALSE,
            .largePoints = VK_FALSE,
            .alphaToOne = VK_FALSE,
            .multiViewport = VK_FALSE,
            .samplerAnisotropy = VK_FALSE,
            .textureCompressionETC2 = VK_FALSE,
            .textureCompressionASTC_LDR = VK_FALSE,
            .textureCompressionBC = VK_FALSE,
            .occlusionQueryPrecise = VK_FALSE,
            .pipelineStatisticsQuery = VK_FALSE,
            .vertexPipelineStoresAndAtomics = VK_FALSE,
            .fragmentStoresAndAtomics = VK_FALSE,
            .shaderTessellationAndGeometryPointSize = VK_FALSE,
            .shaderImageGatherExtended = VK_FALSE,
            .shaderStorageImageExtendedFormats = VK_FALSE,
            .shaderStorageImageMultisample = VK_FALSE,
            .shaderStorageImageReadWithoutFormat = VK_FALSE,
            .shaderStorageImageWriteWithoutFormat = VK_FALSE,
            .shaderUniformBufferArrayDynamicIndexing = VK_FALSE,
            .shaderSampledImageArrayDynamicIndexing = VK_FALSE,
            .shaderStorageBufferArrayDynamicIndexing = VK_FALSE,
            .shaderStorageImageArrayDynamicIndexing = VK_FALSE,
            .shaderClipDistance = VK_FALSE,
            .shaderCullDistance = VK_FALSE,
            .shaderFloat64 = VK_FALSE,
            .shaderInt64 = VK_FALSE,
            .shaderInt16 = VK_FALSE,
            .shaderResourceResidency = VK_FALSE,
            .shaderResourceMinLod = VK_FALSE,
            .sparseBinding = VK_FALSE,
            .sparseResidencyBuffer = VK_FALSE,
            .sparseResidencyImage2D = VK_FALSE,
            .sparseResidencyImage3D = VK_FALSE,
            .sparseResidency2Samples = VK_FALSE,
            .sparseResidency4Samples = VK_FALSE,
            .sparseResidency8Samples = VK_FALSE,
            .sparseResidency16Samples = VK_FALSE,
            .sparseResidencyAliased = VK_FALSE,
            .variableMultisampleRate = VK_FALSE,
            .inheritedQueries = VK_FALSE
        };

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
            .pEnabledFeatures = &deviceFeatures
        };

        ASSERT_THROW(vkCreateDevice(physicalDevice.device, &deviceInfo, nullptr, &device) == VK_SUCCESS,
                     CantCreateError,
                     "Failed to create VkDevice");

        for (uint32_t i = 0; i < queueFamilies.size(); i++)
            for (uint32_t j = 0; j < queueFamilies[i].size(); j++)
                vkGetDeviceQueue(device, i, j, &queueFamilies[i][j].queue);

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
            .vkGetMemoryWin32HandleKHR = nullptr
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
            .vulkanApiVersion = renderingContext->getInstanceApiVersion(),
            .pTypeExternalMemoryHandleTypes = nullptr
        };
        ASSERT_THROW(vmaCreateAllocator(&allocatorInfo, &allocator) == VK_SUCCESS,
                     CantCreateError,
                     "Call to vmaCreateAllocator failed.");
    }

    VkSampleCountFlagBits VulkanRenderingDevice::findClosestSupportedSampleCount(const ImageSamples &samples) const {
        const auto limits = physicalDevice.properties.limits;

        const VkSampleCountFlags flags = limits.framebufferColorSampleCounts & limits.framebufferDepthSampleCounts;
        if (flags & toVkSampleCountFlagBits(samples))
            return toVkSampleCountFlagBits(samples);

        VkSampleCountFlagBits sampleCount = toVkSampleCountFlagBits(samples);
        while (sampleCount > VK_SAMPLE_COUNT_1_BIT) {
            if (flags & sampleCount) {
                return sampleCount;
            }

            sampleCount = static_cast<VkSampleCountFlagBits>(sampleCount >> 1);
        }

        return VK_SAMPLE_COUNT_1_BIT;
    }

    VulkanRenderingDevice::VulkanRenderingDevice(
        const std::shared_ptr<VulkanRenderingContext> &renderingContext,
        const uint32_t deviceIndex
    ) : RenderingDevice(),
        enabledFeatures(),
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
    }

    Swapchain *VulkanRenderingDevice::createSwapchain(Surface *surface) {
        DEBUG_ASSERT(surface != nullptr);

        const auto vkSurface = reinterpret_cast<VulkanSurface *>(surface);

        uint32_t formatCount;
        ASSERT_THROW(
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice.device, vkSurface->surface, &formatCount, nullptr)
            == VK_SUCCESS,
            CantCreateError,
            "Call to vkGetPhysicalDeviceSurfaceFormatsKHR failed.");
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        ASSERT_THROW(
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice.device, vkSurface->surface, &formatCount,
                formats.data()) == VK_SUCCESS,
            CantCreateError,
            "Call to vkGetPhysicalDeviceSurfaceFormatsKHR failed.");

        auto *swapchain = new VulkanSwapchain();

        swapchain->surface = vkSurface;
        swapchain->format = VK_FORMAT_UNDEFINED;
        swapchain->colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

        if (formatCount == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
            swapchain->format = VK_FORMAT_B8G8R8A8_SRGB;
            swapchain->colorSpace = formats[0].colorSpace;
        } else if (formatCount > 0) {
            constexpr VkFormat preferredFormat = VK_FORMAT_B8G8R8A8_UNORM;
            constexpr VkFormat alternativeFormat = VK_FORMAT_R8G8B8A8_UNORM;

            for (uint32_t i = 0; i < formatCount; i++) {
                if (formats[i].format == preferredFormat || formats[i].format == alternativeFormat) {
                    swapchain->format = formats[i].format;
                    swapchain->colorSpace = formats[i].colorSpace;

                    if (formats[i].format == preferredFormat)
                        break;
                }
            }
        }

        ASSERT_THROW(swapchain->format != VK_FORMAT_UNDEFINED, CantCreateError,
                     "Surface does not have any supported formats.");

        return swapchain;
    }

    void VulkanRenderingDevice::resizeSwapchain(CommandQueue *commandQueue, Swapchain *swapchain,
                                                const uint32_t imageCount) {
        DEBUG_ASSERT(commandQueue != nullptr);
        DEBUG_ASSERT(swapchain != nullptr);

        const auto vkSwapchain = reinterpret_cast<VulkanSwapchain *>(swapchain);

        const auto surface = vkSwapchain->surface;
        const auto surfaceCapabilities = physicalDevice.getSurfaceCapabilities(surface->surface);

        VkSwapchainCreateInfoKHR swapchainInfo{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .surface = surface->surface,
            .minImageCount = std::max(surfaceCapabilities.minImageCount, imageCount),
            .imageFormat = vkSwapchain->format,
            .imageColorSpace = vkSwapchain->colorSpace,
            .imageExtent = {
                .width = static_cast<uint32_t>(surface->resolution.x),
                .height = static_cast<uint32_t>(surface->resolution.y)
            },
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            // TODO: Add support for transparent frames, useful for e.g. splash screens with transparent backgrounds.
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = VK_PRESENT_MODE_FIFO_KHR,
            .clipped = VK_TRUE,
            .oldSwapchain = nullptr
        };

        // TODO: Queue family index and image sharing mode should be set dynamically if the graphics queue family
        //  and present queue family are not the same.
        // if (device->getGraphicsQueueFamily().index != device->getPresentQueueFamily().index) {
        //     const std::vector indices = {
        //         device->getGraphicsQueueFamily().index,
        //         device->getPresentQueueFamily().index
        //     };
        //
        //     info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        //     info.queueFamilyIndexCount = indices.size();
        //     info.pQueueFamilyIndices = indices.data();
        // } else {
        //     info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        //     info.queueFamilyIndexCount = 0;
        //     info.pQueueFamilyIndices = nullptr;
        // }

        std::vector<VkPresentModeKHR> supportedPresentModes{};
        uint32_t presentModeCount;
        ASSERT_THROW(
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice.device, surface->surface, &presentModeCount,
                nullptr) == VK_SUCCESS,
            CantCreateError,
            "Call to vkGetPhysicalDeviceSurfacePresentModesKHR failed.");
        supportedPresentModes.resize(presentModeCount);
        ASSERT_THROW(
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice.device, surface->surface, &presentModeCount,
                supportedPresentModes.data()) == VK_SUCCESS,
            CantCreateError,
            "Call to vkGetPhysicalDeviceSurfacePresentModesKHR failed.");

        switch (surface->vsyncMode) {
            case VSyncMode::Disabled:
                swapchainInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                break;

            case VSyncMode::Enabled:
                swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
                break;

            case VSyncMode::Adaptive:
                swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
                break;

            case VSyncMode::Mailbox:
                swapchainInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
        }

        if (std::ranges::find(supportedPresentModes.begin(), supportedPresentModes.end(), swapchainInfo.presentMode) ==
            supportedPresentModes.end()) {
            spdlog::warn("Requested VSync mode is not available. Falling back to vsync mode enabled.");
            swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        }

        ASSERT_THROW(vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &vkSwapchain->swapchain) == VK_SUCCESS,
                     CantCreateError,
                     "Call to vkCreateSwapchainKHR failed.");

        uint32_t swapchainImageCount;
        ASSERT_THROW(
            vkGetSwapchainImagesKHR(device, vkSwapchain->swapchain, &swapchainImageCount, nullptr) == VK_SUCCESS,
            CantCreateError,
            "Call to vkGetSwapchainImagesKHR failed.");
        vkSwapchain->resolveImages.resize(swapchainImageCount);
        vkSwapchain->resolveImageViews.resize(swapchainImageCount);
        ASSERT_THROW(
            vkGetSwapchainImagesKHR(device, vkSwapchain->swapchain, &swapchainImageCount, vkSwapchain->resolveImages.
                data()) == VK_SUCCESS,
            CantCreateError,
            "Call to vkGetSwapchainImagesKHR failed.");

        vkSwapchain->colorTargets.resize(swapchainImageCount);
        vkSwapchain->depthTargets.resize(swapchainImageCount);
        for (uint32_t i = 0; i < swapchainImageCount; i++) {
            VkImageViewCreateInfo imageViewInfo{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .image = vkSwapchain->resolveImages[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = swapchainInfo.imageFormat,
                .components = {
                    .r = VK_COMPONENT_SWIZZLE_R,
                    .g = VK_COMPONENT_SWIZZLE_G,
                    .b = VK_COMPONENT_SWIZZLE_B,
                    .a = VK_COMPONENT_SWIZZLE_A
                },
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };
            ASSERT_THROW(
                vkCreateImageView(device, &imageViewInfo, nullptr, &vkSwapchain->resolveImageViews[i]) == VK_SUCCESS,
                CantCreateError,
                "Failed to create image views for swap chain acquired images.");

            vkSwapchain->colorTargets[i] = reinterpret_cast<VulkanImage *>(createImage(
                {
                    .format = static_cast<DataFormat>(vkSwapchain->format - 1),
                    .width = swapchainInfo.imageExtent.width,
                    .height = swapchainInfo.imageExtent.height,
                    .depth = 1,
                    .layerCount = 1,
                    .mipmapCount = 1,
                    .type = ImageType::TwoD,
                    .samples = ImageSamples::One,
                    .usage = ImageUsage::ColorAttachment | ImageUsage::CopySource
                },
                {
                    .format = static_cast<DataFormat>(vkSwapchain->format - 1),
                    .swizzleRed = ImageSwizzle::Identity,
                    .swizzleGreen = ImageSwizzle::Identity,
                    .swizzleBlue = ImageSwizzle::Identity,
                    .swizzleAlpha = ImageSwizzle::Identity
                }
            ));
            vkSwapchain->depthTargets[i] = reinterpret_cast<VulkanImage *>(createImage(
                {
                    // TODO: Actually check for a supported depth format instead of blindly picking our preferred one.
                    .format = D32_SFLOAT_S8_UINT,
                    .width = swapchainInfo.imageExtent.width,
                    .height = swapchainInfo.imageExtent.height,
                    .depth = 1,
                    .layerCount = 1,
                    .mipmapCount = 1,
                    .type = ImageType::TwoD,
                    .samples = ImageSamples::One,
                    .usage = ImageUsage::DepthStencilAttachment
                },
                {
                    .format = D32_SFLOAT_S8_UINT,
                    .swizzleRed = ImageSwizzle::Identity,
                    .swizzleGreen = ImageSwizzle::Identity,
                    .swizzleBlue = ImageSwizzle::Identity,
                    .swizzleAlpha = ImageSwizzle::Identity
                }
            ));
        }
    }

    void VulkanRenderingDevice::destroySwapchain(Swapchain *swapchain) {
        DEBUG_ASSERT(swapchain != nullptr);

        const auto vkSwapchain = reinterpret_cast<VulkanSwapchain *>(swapchain);

        for (uint32_t i = 0; i < vkSwapchain->resolveImages.size(); i++) {
            destroyImage(vkSwapchain->colorTargets[i]);
            destroyImage(vkSwapchain->depthTargets[i]);
            vkDestroyImageView(device, vkSwapchain->resolveImageViews[i], nullptr);
        }
        vkDestroySwapchainKHR(device, vkSwapchain->swapchain, nullptr);
        delete vkSwapchain;
    }

    uint32_t VulkanRenderingDevice::getQueueFamily(QueueFamilyFlags queueFamilyFlags, Surface *surface) {
        VkQueueFlags pickedQueueFlags = VK_QUEUE_FLAG_BITS_MAX_ENUM;
        uint32_t pickedQueueFamilyIndex = std::numeric_limits<uint32_t>::max();

        for (uint32_t i = 0; i < queueFamilies.size(); i++) {
            if (queueFamilies[i].empty())
                continue;

            if (surface != nullptr && !renderingContext->supportsPresent(
                    physicalDevice.device, i, reinterpret_cast<VulkanSurface *>(surface)))
                continue;

            const VkQueueFlags optionQueueFlags = physicalDevice.queueFamilies[i].properties.queueFlags;
            const bool includesAllBits = static_cast<QueueFamilyFlags>(optionQueueFlags) & queueFamilyFlags;
            if (const bool preferLessBits = optionQueueFlags < pickedQueueFlags;
                includesAllBits && preferLessBits) {
                pickedQueueFamilyIndex = i;
                pickedQueueFlags = optionQueueFlags;
            }
        }

        ASSERT_THROW(pickedQueueFamilyIndex <= queueFamilies.size(), CantCreateError,
                     "Failed to find suitable queue family");

        return pickedQueueFamilyIndex;
    }

    Fence *VulkanRenderingDevice::createFence() {
        const auto o = new VulkanFence();

        constexpr VkFenceCreateInfo fenceInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0
        };

        ASSERT_THROW(vkCreateFence(device, &fenceInfo, nullptr, &o->fence) == VK_SUCCESS,
                     CantCreateError,
                     "Call to vkCreateFence failed.");

        return o;
    }

    void VulkanRenderingDevice::waitOnFence(const Fence *fence) {
        const auto o = reinterpret_cast<const VulkanFence *>(fence);
        vkWaitForFences(device, 1, &o->fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
    }

    void VulkanRenderingDevice::destroyFence(Fence *fence) {
        const auto o = reinterpret_cast<VulkanFence *>(fence);
        vkDestroyFence(device, o->fence, nullptr);
        delete o;
    }

    Semaphore *VulkanRenderingDevice::createSemaphore() {
        const auto o = new VulkanSemaphore();

        constexpr VkSemaphoreCreateInfo semaphoreInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0
        };
        ASSERT_THROW(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &o->semaphore) == VK_SUCCESS,
                     CantCreateError,
                     "Call to vkCreateSemaphore failed.");

        return o;
    }

    void VulkanRenderingDevice::destroySemaphore(Semaphore *semaphore) {
        const auto o = reinterpret_cast<VulkanSemaphore *>(semaphore);
        vkDestroySemaphore(device, o->semaphore, nullptr);
        delete o;
    }

    CommandPool *VulkanRenderingDevice::createCommandPool(const uint32_t queueFamily, CommandBufferType type) {
        const VkCommandPoolCreateInfo commandPoolInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = queueFamily
        };

        VkCommandPool commandPool;
        ASSERT_THROW(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool) == VK_SUCCESS,
                     CantCreateError, "Call to vkCreateCommandPool failed.");
        const auto o = new VulkanCommandPool{};
        o->pool = commandPool;
        o->type = type;
        o->queueFamily = queueFamily;
        return o;
    }

    void VulkanRenderingDevice::resetCommandPool(CommandPool *pool) {
        const auto *o = reinterpret_cast<VulkanCommandPool *>(pool);
        ASSERT_THROW(vkResetCommandPool(device, o->pool, 0) == VK_SUCCESS, CantCreateError,
                     "Call to vkResetCommandPool failed.");
    }

    void VulkanRenderingDevice::destroyCommandPool(CommandPool *pool) {
        const auto *o = reinterpret_cast<VulkanCommandPool *>(pool);
        vkDestroyCommandPool(device, o->pool, nullptr);
        delete o;
    }

    CommandBuffer *VulkanRenderingDevice::createCommandBuffer(CommandPool *pool) {
        const auto *p = reinterpret_cast<VulkanCommandPool *>(pool);

        const VkCommandBufferAllocateInfo commandBufferInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = p->pool,
            .level = static_cast<VkCommandBufferLevel>(p->type),
            .commandBufferCount = 1
        };

        VkCommandBuffer commandBuffer;
        ASSERT_THROW(vkAllocateCommandBuffers(device, &commandBufferInfo, &commandBuffer) == VK_SUCCESS,
                     CantCreateError, "Call to vkAllocateCommandBuffers failed.");
        const auto o = new VulkanCommandBuffer{};
        o->commandBuffer = commandBuffer;

        return o;
    }

    void VulkanRenderingDevice::beginCommandBuffer(CommandBuffer *commandBuffer) {
        const auto o = reinterpret_cast<VulkanCommandBuffer *>(commandBuffer);

        constexpr VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr
        };
        ASSERT_THROW(vkBeginCommandBuffer(o->commandBuffer, &beginInfo) == VK_SUCCESS,
                     CantCreateError,
                     "Call to vkBeginCommandBuffer failed.");
    }

    void VulkanRenderingDevice::endCommandBuffer(CommandBuffer *commandBuffer) {
        const auto o = reinterpret_cast<VulkanCommandBuffer *>(commandBuffer);
        vkEndCommandBuffer(o->commandBuffer);
    }

    CommandQueue *VulkanRenderingDevice::createCommandQueue() {
        auto commandQueue = new VulkanCommandQueue();

        // TODO

        return commandQueue;
    }

    void VulkanRenderingDevice::executeCommandQueueAndPresent(CommandQueue *commandQueue,
                                                              const std::vector<Semaphore *> &waitSemaphores,
                                                              const std::vector<CommandBuffer *> &commandBuffers,
                                                              const std::vector<Semaphore *> &semaphores,
                                                              Fence *fence,
                                                              const std::vector<Swapchain *> &swapchains) {
        const auto vkCommandQueue = reinterpret_cast<VulkanCommandQueue *>(commandQueue);

        const VkQueue queue = VK_NULL_HANDLE;

        std::vector<VkSemaphore> vkWaitSemaphores{};
        vkWaitSemaphores.reserve(waitSemaphores.size());
        for (const auto &semaphore: waitSemaphores)
            vkWaitSemaphores.push_back(reinterpret_cast<VulkanSemaphore *>(semaphore)->semaphore);

        std::vector<VkCommandBuffer> vkCommandBuffers{};
        vkCommandBuffers.reserve(commandBuffers.size());
        for (const auto &commandBuffer: commandBuffers)
            vkCommandBuffers.push_back(reinterpret_cast<VulkanCommandBuffer *>(commandBuffer)->commandBuffer);

        VkSemaphoreWaitFlags waitFlags = VK_SEMAPHORE_WAIT_ANY_BIT;

        const VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = static_cast<uint32_t>(vkWaitSemaphores.size()),
            .pWaitSemaphores = vkWaitSemaphores.data(),
            .pWaitDstStageMask = &waitFlags,
            .commandBufferCount = static_cast<uint32_t>(vkCommandBuffers.size()),
            .pCommandBuffers = vkCommandBuffers.data(),
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr
        };

        vkQueueSubmit(queue, 1, &submitInfo, nullptr);

        std::vector<VkSwapchainKHR> vkSwapchains{};
        vkSwapchains.reserve(swapchains.size());
        for (const auto &swapchain: swapchains)
            vkSwapchains.push_back(reinterpret_cast<VulkanSwapchain *>(swapchain)->swapchain);

        VkPresentInfoKHR presentInfo{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = nullptr,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .swapchainCount = static_cast<uint32_t>(vkSwapchains.size()),
            .pSwapchains = vkSwapchains.data(),
            .pImageIndices = nullptr,
            .pResults = nullptr
        };

        vkQueuePresentKHR(queue, &presentInfo);

        // TODO: Present to swapchain
    }

    void VulkanRenderingDevice::destroyCommandQueue(CommandQueue *commandQueue) {
        const auto o = reinterpret_cast<VulkanCommandQueue *>(commandQueue);

        // TODO: Destroy Vulkan objects

        delete o;
    }

    Buffer *VulkanRenderingDevice::createBuffer(const BufferUsage usage, const uint32_t count, const uint32_t stride) {
        if (count <= 0)
            throw std::runtime_error("Count cannot be equal to or less than 0");
        if (stride <= 0)
            throw std::runtime_error("Stride cannot be equal to or less than 0");

        VmaAllocationCreateFlags allocationFlags = 0;
        VkBufferUsageFlags bufferUsageFlags = 0;
        VkMemoryPropertyFlags requiredFlags = 0;

        if (usage & BufferUsage::Vertex)
            bufferUsageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        if (usage & BufferUsage::Index)
            bufferUsageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        if (usage & BufferUsage::CopySource) {
            allocationFlags |= VMA_ALLOCATION_CREATE_MAPPED_BIT |
                    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            bufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }

        if (usage & BufferUsage::CopyDestination)
            bufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        if (usage & BufferUsage::Uniform) {
            allocationFlags |= VMA_ALLOCATION_CREATE_MAPPED_BIT |
                    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            bufferUsageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            requiredFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        }

        const VkBufferCreateInfo bufferCreateInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .size = count * stride,
            .usage = bufferUsageFlags,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr
        };

        const VmaAllocationCreateInfo allocationCreateInfo = {
            .flags = allocationFlags,
            .usage = VMA_MEMORY_USAGE_AUTO,
            .requiredFlags = requiredFlags,
            .preferredFlags = 0,
            .memoryTypeBits = 0,
            .pool = nullptr,
            .pUserData = nullptr,
            .priority = 0.0f
        };

        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo;
        ASSERT_THROW(
            vmaCreateBuffer(
                allocator,
                &bufferCreateInfo,
                &allocationCreateInfo,
                &buffer,
                &allocation,
                &allocationInfo
            ) == VK_SUCCESS,
            CantCreateError,
            "Failed to create buffer"
        );

        return new VulkanBuffer(
            usage,
            count,
            stride,
            buffer,
            allocation
        );
    }

    void VulkanRenderingDevice::destroyBuffer(Buffer *buffer) {
        const auto o = dynamic_cast<VulkanBuffer *>(buffer);
        vmaDestroyBuffer(allocator, o->buffer, o->allocation);
        delete o;
    }

    Image *VulkanRenderingDevice::createImage(const ImageFormat &format, const ImageView &view) {
        VkImageCreateInfo imageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = toVkImageType(format.type),
            .format = toVkDataFormat[format.format],
            .extent = {
                .width = format.width,
                .height = format.height,
                .depth = format.depth
            },
            .mipLevels = format.mipmapCount,
            .arrayLayers = format.layerCount,
            .samples = findClosestSupportedSampleCount(format.samples),
            .tiling = format.usage & ImageUsage::CpuRead ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL,
            .usage = 0,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        };

        if (format.type == ImageType::Cube || format.type == ImageType::CubeArray)
            imageCreateInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        if (format.usage & ImageUsage::Sampling)
            imageCreateInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        if (format.usage & ImageUsage::Storage)
            imageCreateInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
        if (format.usage & ImageUsage::ColorAttachment)
            imageCreateInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (format.usage & ImageUsage::DepthStencilAttachment)
            imageCreateInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        if (format.usage & ImageUsage::InputAttachment)
            imageCreateInfo.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        if (format.usage & ImageUsage::Update)
            imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        if (format.usage & ImageUsage::CopySource)
            imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        if (format.usage & ImageUsage::CopyDestination)
            imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo allocationCreateInfo{
            .flags = static_cast<VmaAllocationCreateFlags>(
                format.usage & ImageUsage::CpuRead
                    ? VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
                    : 0
            ),
            .usage = VMA_MEMORY_USAGE_AUTO,
            .requiredFlags = 0,
            .preferredFlags = 0,
            .memoryTypeBits = 0,
            .pool = nullptr,
            .pUserData = nullptr,
            .priority = 0.0f
        };

        if (format.usage & ImageUsage::Transient) {
            uint32_t memoryTypeIndex = 0;
            VmaAllocationCreateInfo lazyMemoryRequirements = allocationCreateInfo;
            lazyMemoryRequirements.usage = VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;
            if (const VkResult result = vmaFindMemoryTypeIndex(allocator, UINT32_MAX, &lazyMemoryRequirements,
                                                               &memoryTypeIndex);
                result == VK_SUCCESS) {
                allocationCreateInfo = lazyMemoryRequirements;
                imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
                imageCreateInfo.usage &= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                        | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                        VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
            } else {
                allocationCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            }
        } else {
            allocationCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }

        // TODO: Handle small allocations

        VkImage image;
        VmaAllocation allocation;
        ASSERT_THROW(
            vmaCreateImage(
                allocator,
                &imageCreateInfo,
                &allocationCreateInfo,
                &image,
                &allocation,
                nullptr
            ) == VK_SUCCESS,
            CantCreateError,
            "Failed to create image"
        );

        VkImageView imageView;
        const VkImageViewCreateInfo imageViewInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = imageCreateInfo.format,
            .components = {
                .r = static_cast<VkComponentSwizzle>(view.swizzleRed),
                .g = static_cast<VkComponentSwizzle>(view.swizzleGreen),
                .b = static_cast<VkComponentSwizzle>(view.swizzleBlue),
                .a = static_cast<VkComponentSwizzle>(view.swizzleAlpha)
            },
            .subresourceRange = {
                .aspectMask = static_cast<VkImageAspectFlags>(format.usage & ImageUsage::DepthStencilAttachment
                                                                  ? VK_IMAGE_ASPECT_DEPTH_BIT |
                                                                    VK_IMAGE_ASPECT_STENCIL_BIT
                                                                  : VK_IMAGE_ASPECT_COLOR_BIT),
                .baseMipLevel = 0,
                .levelCount = imageCreateInfo.mipLevels,
                .baseArrayLayer = 0,
                .layerCount = imageCreateInfo.arrayLayers
            }
        };

        if (const auto error = vkCreateImageView(device, &imageViewInfo, nullptr, &imageView);
            error != VK_SUCCESS) {
            vmaDestroyImage(allocator, image, allocation);
            ASSERT_THROW(error == VK_SUCCESS, CantCreateError, "Call to vkCreateImageView failed.");
        }

        const auto o = new VulkanImage();
        o->format = format;
        o->view = view;
        o->image = image;
        o->imageView = imageView;
        o->allocation = allocation;
        return o;
    }

    std::byte *VulkanRenderingDevice::mapImage(Image *image) {
        const auto o = reinterpret_cast<VulkanImage *>(image);
        std::byte *data;
        vmaMapMemory(allocator, o->allocation, reinterpret_cast<void **>(&data));
        return data;
    }

    void VulkanRenderingDevice::unmapImage(Image *image) {
        const auto o = reinterpret_cast<VulkanImage *>(image);
        vmaUnmapMemory(allocator, o->allocation);
    }

    void VulkanRenderingDevice::destroyImage(Image *image) {
        const auto o = reinterpret_cast<VulkanImage *>(image);
        vkDestroyImageView(device, o->imageView, nullptr);
        vmaDestroyImage(allocator, o->image, o->allocation);
        delete o;
    }

    Sampler *VulkanRenderingDevice::createSampler(SamplerState state) {
        const VkSamplerCreateInfo samplerInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .magFilter = state.mag == SamplerFilter::Linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST,
            .minFilter = state.min == SamplerFilter::Linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST,
            .mipmapMode = state.mip == SamplerFilter::Linear
                              ? VK_SAMPLER_MIPMAP_MODE_LINEAR
                              : VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .addressModeU = static_cast<VkSamplerAddressMode>(state.u),
            .addressModeV = static_cast<VkSamplerAddressMode>(state.v),
            .addressModeW = static_cast<VkSamplerAddressMode>(state.w),
            .mipLodBias = state.lodBias,
            .anisotropyEnable = state.useAnisotropy && physicalDevice.features.samplerAnisotropy,
            .maxAnisotropy = state.maxAnisotropy,
            .compareEnable = state.enableCompare,
            .compareOp = static_cast<VkCompareOp>(state.compareOperator),
            .minLod = state.minLod,
            .maxLod = state.maxLod,
            .borderColor = static_cast<VkBorderColor>(state.borderColor),
            .unnormalizedCoordinates = state.unnormalizedCoordinates ? VK_TRUE : VK_FALSE
        };

        VkSampler sampler;
        ASSERT_THROW(vkCreateSampler(device, &samplerInfo, nullptr, &sampler) == VK_SUCCESS,
                     CantCreateError, "Call to vkCreateSampler failed.");

        const auto o = new VulkanSampler{};
        o->state = state;
        o->sampler = sampler;
        return o;
    }

    void VulkanRenderingDevice::destroySampler(Sampler *sampler) {
        const auto o = reinterpret_cast<VulkanSampler *>(sampler);
        vkDestroySampler(device, o->sampler, nullptr);
        delete o;
    }

    Shader *VulkanRenderingDevice::createShaderFromSpirv(const std::string &name,
                                                         const std::vector<ShaderStageData> &stages) {
        const auto o = new VulkanShader();
        if (!reflectShader(stages, o)) {
            delete o;
            ASSERT_THROW(false, CantCreateError, "Shader reflection failed.");
        }

        o->name = name;

        for (const auto &stage: o->pushConstantStages)
            o->pushConstantStageFlags |= toVkShaderStageFlag(stage);

        for (const auto &[stage, spirv]: stages) {
            const auto stageFlag = toVkShaderStageFlag(stage);

            const VkShaderModuleCreateInfo shaderModuleInfo{
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .codeSize = spirv.size(),
                .pCode = reinterpret_cast<const uint32_t *>(spirv.data())
            };

            VkShaderModule module;
            ASSERT_THROW(vkCreateShaderModule(device, &shaderModuleInfo, nullptr, &module) == VK_SUCCESS,
                         CantCreateError,
                         "Call to vkCreateShaderModule failed.");

            std::vector<VkDescriptorSetLayoutBinding> layoutBindings{};
            layoutBindings.reserve(o->uniformSets.size());

            for (const auto &uniformSet: o->uniformSets) {
                VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;

                switch (uniformSet.type) {
                    case ShaderUniformType::Sampler:
                        descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                        break;

                    case ShaderUniformType::CombinedImageSampler:
                        descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        break;

                    case ShaderUniformType::UniformBuffer:
                        descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        break;
                }

                const VkDescriptorSetLayoutBinding layoutBinding = {
                    .binding = uniformSet.binding,
                    .descriptorType = descriptorType,
                    .descriptorCount = 1,
                    .stageFlags = stageFlag,
                    .pImmutableSamplers = nullptr
                };
                layoutBindings.push_back(layoutBinding);
            }

            const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .bindingCount = static_cast<uint32_t>(layoutBindings.size()),
                .pBindings = layoutBindings.data()
            };

            VkDescriptorSetLayout descriptorSetLayout = nullptr;
            ASSERT_THROW(
                vkCreateDescriptorSetLayout(device, &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout) ==
                VK_SUCCESS,
                CantCreateError,
                "Call to vkCreateDescriptorSetLayout failed.");
            o->descriptorSetLayouts.push_back(descriptorSetLayout);

            o->shaderStageInfos.push_back({
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = static_cast<VkShaderStageFlagBits>(stageFlag),
                .module = module,
                .pName = nullptr,
                .pSpecializationInfo = nullptr
            });
        }

        const VkPushConstantRange pushConstantRange{
            .stageFlags = o->pushConstantStageFlags,
            .offset = 0,
            .size = o->pushConstantSize
        };

        const VkPipelineLayoutCreateInfo pipelineLayoutInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = static_cast<uint32_t>(o->descriptorSetLayouts.size()),
            .pSetLayouts = o->descriptorSetLayouts.data(),
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = pushConstantRange.size > 0 ? &pushConstantRange : nullptr
        };

        VkPipelineLayout pipelineLayout;
        ASSERT_THROW(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) == VK_SUCCESS,
                     CantCreateError,
                     "Call to vkCreatePipelineLayout failed.");
        o->pipelineLayout = pipelineLayout;

        return o;
    }

    void VulkanRenderingDevice::destroyShaderModules(Shader *shader) {
        const auto o = reinterpret_cast<VulkanShader *>(shader);
        for (const auto &stageInfo: o->shaderStageInfos)
            vkDestroyShaderModule(device, stageInfo.module, nullptr);
        o->shaderStageInfos.clear();
    }

    void VulkanRenderingDevice::destroyShader(Shader *shader) {
        const auto o = reinterpret_cast<VulkanShader *>(shader);

        destroyShaderModules(o);
        for (const auto &descriptorSetLayout: o->descriptorSetLayouts)
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        vkDestroyPipelineLayout(device, o->pipelineLayout, nullptr);

        delete o;
    }

    GraphicsCard VulkanRenderingDevice::getPhysicalDevice() const {
        return physicalDevice;
    }
}
