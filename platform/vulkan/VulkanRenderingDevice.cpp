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
        requestedExtensions[VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME] = false;
        requestedExtensions[VK_KHR_MAINTENANCE_2_EXTENSION_NAME] = false;

#ifdef DEBUG_ENABLED
        requestedExtensions[VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME] = false;
        requestedExtensions[VK_EXT_DEVICE_FAULT_EXTENSION_NAME] = false;
        requestedExtensions[VK_EXT_DEBUG_MARKER_EXTENSION_NAME] = false;
#endif

        uint32_t extensionCount = 0;
        if (vkEnumerateDeviceExtensionProperties(physicalDevice.device, nullptr, &extensionCount, nullptr) !=
            VK_SUCCESS)
            error<CantCreateError>("Call to vkEnumerateDeviceExtensionProperties failed.");
        std::vector<VkExtensionProperties> availableExtensions{extensionCount};
        if (vkEnumerateDeviceExtensionProperties(physicalDevice.device, nullptr, &extensionCount, availableExtensions.
                                                 data()) != VK_SUCCESS)
            error<CantCreateError>("Call to vkEnumerateDeviceExtensionProperties failed.");

        for (uint32_t i = 0; i < extensionCount; i++) {
            const auto &extensionName = availableExtensions[i].extensionName;
            spdlog::trace("VULKAN: Found device extension {}.", extensionName);
            if (requestedExtensions.contains(extensionName))
                enabledExtensionNames.push_back(strdup(extensionName));
        }

        for (const auto &[extensionName, required]: requestedExtensions) {
            if (std::ranges::find(enabledExtensionNames.begin(), enabledExtensionNames.end(), extensionName) ==
                enabledExtensionNames.end()) {
                if (required)
                    error<CantCreateError>("Required extension \"" + extensionName + "\" was not found");

                spdlog::debug("Optional extension {} was not found.", extensionName);
            }
        }
    }

    void VulkanRenderingDevice::checkFeatures() const {
        if (!physicalDevice.features.imageCubeArray)
            error<CantCreateError>("Device lacks image cube array feature.");
        if (!physicalDevice.features.independentBlend)
            error<CantCreateError>("Device lacks independent blend feature.");
    }

    void VulkanRenderingDevice::checkCapabilities() {
        if (std::ranges::find(enabledExtensionNames, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME) != enabledExtensionNames.
            end())
            enabledFeatures.dynamicRendering = true;
        if (std::ranges::find(enabledExtensionNames, VK_EXT_DEVICE_FAULT_EXTENSION_NAME) != enabledExtensionNames.end())
            enabledFeatures.deviceFault = true;
        if (std::ranges::find(enabledExtensionNames, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME) != enabledExtensionNames.
            end())
            enabledFeatures.synchronization2 = true;
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
            .dynamicRendering = enabledFeatures.dynamicRendering ? VK_TRUE : VK_FALSE
        };

        VkPhysicalDeviceSynchronization2Features synchronization2Features{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
            .pNext = nullptr,
            .synchronization2 = enabledFeatures.synchronization2 ? VK_TRUE : VK_FALSE
        };
        dynamicRenderingFeatures.pNext = &synchronization2Features;

        VkPhysicalDeviceFaultFeaturesEXT faultFeatures{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT,
            .pNext = nullptr,
            .deviceFault = enabledFeatures.deviceFault ? VK_TRUE : VK_FALSE,
            .deviceFaultVendorBinary = enabledFeatures.deviceFault ? VK_TRUE : VK_FALSE
        };
        synchronization2Features.pNext = &faultFeatures;

        auto enabledExtensions = std::vector<const char *>{};
        enabledExtensions.reserve(enabledExtensionNames.size());
        for (auto i = 0; i < enabledExtensionNames.size(); i++)
            enabledExtensions.push_back(enabledExtensionNames[i].c_str());

        const VkDeviceCreateInfo deviceInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &dynamicRenderingFeatures,
            .flags = 0,
            .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
            .pQueueCreateInfos = queueCreateInfos.data(),
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size()),
            .ppEnabledExtensionNames = enabledExtensions.data(),
            .pEnabledFeatures = &physicalDevice.features
        };

        if (vkCreateDevice(physicalDevice.device, &deviceInfo, nullptr, &device) != VK_SUCCESS)
            error<CantCreateError>("Failed to create VkDevice");

        queueFamilies.resize(queueCreateInfos.size());
        for (uint32_t i = 0; i < queueFamilies.size(); i++) {
            queueFamilies[i].resize(queueCreateInfos[i].queueCount);
            for (uint32_t j = 0; j < queueFamilies[i].size(); j++) {
                vkGetDeviceQueue(device, queueCreateInfos[i].queueFamilyIndex, j, &queueFamilies[i][j].queue);
            }
        }

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
        if (vmaCreateAllocator(&allocatorInfo, &allocator) != VK_SUCCESS)
            error<CantCreateError>("Call to vmaCreateAllocator failed.");
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
        VulkanRenderingContext *renderingContext,
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

        const auto vkSurface = dynamic_cast<VulkanSurface *>(surface);

        uint32_t formatCount;
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice.device, vkSurface->surface, &formatCount, nullptr)
            != VK_SUCCESS)
            error<CantCreateError>("Call to vkGetPhysicalDeviceSurfaceFormatsKHR failed.");
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice.device, vkSurface->surface, &formatCount,
                                                 formats.data()) != VK_SUCCESS)
            error<CantCreateError>("Call to vkGetPhysicalDeviceSurfaceFormatsKHR failed.");

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

        if (swapchain->format == VK_FORMAT_UNDEFINED)
            error<CantCreateError>("Surface does not have any supported formats.");

        return swapchain;
    }

    void VulkanRenderingDevice::resizeSwapchain(CommandQueue *commandQueue, Swapchain *swapchain,
                                                const uint32_t imageCount) {
        DEBUG_ASSERT(commandQueue != nullptr);
        DEBUG_ASSERT(swapchain != nullptr);

        const auto vkSwapchain = dynamic_cast<VulkanSwapchain *>(swapchain);
        _destroySwapchain(vkSwapchain);

        const auto surface = vkSwapchain->surface;
        if (renderingContext->supportsPresent(physicalDevice.device, commandQueue->queueFamilyIndex, surface) != true)
            error<CantCreateError>("Surface is not supported by device.");

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
        //  and present queue family are not the same. In that case the imageSharingMode should be
        //  VK_SHARING_MODE_CONCURRENT and both the graphics and present queue indices should be passed to
        //  pQueueFamilyIndices.

        std::vector<VkPresentModeKHR> supportedPresentModes{};
        uint32_t presentModeCount;
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice.device, surface->surface, &presentModeCount,
                                                      nullptr) != VK_SUCCESS)
            error<CantCreateError>("Call to vkGetPhysicalDeviceSurfacePresentModesKHR failed.");
        supportedPresentModes.resize(presentModeCount);
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice.device, surface->surface, &presentModeCount,
                                                      supportedPresentModes.data()) != VK_SUCCESS)
            error<CantCreateError>("Call to vkGetPhysicalDeviceSurfacePresentModesKHR failed.");

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

        if (vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &vkSwapchain->swapchain) != VK_SUCCESS)
            error<CantCreateError>("Call to vkCreateSwapchainKHR failed.");

        uint32_t swapchainImageCount;
        if (vkGetSwapchainImagesKHR(device, vkSwapchain->swapchain, &swapchainImageCount, nullptr) != VK_SUCCESS)
            error<CantCreateError>("Call to vkGetSwapchainImagesKHR failed.");
        vkSwapchain->resolveImages.resize(swapchainImageCount);
        vkSwapchain->resolveImageViews.resize(swapchainImageCount);
        if (vkGetSwapchainImagesKHR(device, vkSwapchain->swapchain, &swapchainImageCount, vkSwapchain->resolveImages.
                                    data()) != VK_SUCCESS)
            error<CantCreateError>("Call to vkGetSwapchainImagesKHR failed.");

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
            if (vkCreateImageView(device, &imageViewInfo, nullptr, &vkSwapchain->resolveImageViews[i]) != VK_SUCCESS)
                error<CantCreateError>("Failed to create image views for swap chain acquired images.");

            vkSwapchain->colorTargets[i] = dynamic_cast<VulkanImage *>(createImage(
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
                    .swizzleRed = ImageSwizzle::Red,
                    .swizzleGreen = ImageSwizzle::Green,
                    .swizzleBlue = ImageSwizzle::Blue,
                    .swizzleAlpha = ImageSwizzle::Alpha
                }
            ));
            vkSwapchain->depthTargets[i] = dynamic_cast<VulkanImage *>(createImage(
                {
                    // TODO: Actually search for a supported depth format instead of blindly picking our preferred one.
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
                    .swizzleRed = ImageSwizzle::Red,
                    .swizzleGreen = ImageSwizzle::Green,
                    .swizzleBlue = ImageSwizzle::Blue,
                    .swizzleAlpha = ImageSwizzle::Alpha
                }
            ));

            // TODO: Transition color targets
        }
    }

    Framebuffer *VulkanRenderingDevice::acquireSwapchainFramebuffer(CommandQueue *commandQueue, Swapchain *swapchain,
                                                                    bool &resizeRequired) {
        DEBUG_ASSERT(commandQueue != nullptr);
        DEBUG_ASSERT(swapchain != nullptr);

        auto vkCommandQueue = dynamic_cast<VulkanCommandQueue *>(commandQueue);
        auto vkSwapchain = dynamic_cast<VulkanSwapchain *>(swapchain);

        vkAcquireNextImageKHR(device, vkSwapchain->swapchain, std::numeric_limits<uint64_t>::max(), VK_NULL_HANDLE,
                              VK_NULL_HANDLE, &vkSwapchain->imageIndex);

        vkSwapchain->framebuffers[vkSwapchain->imageIndex].swapchainAcquired = true;
        return &vkSwapchain->framebuffers[vkSwapchain->imageIndex];
    }

    void VulkanRenderingDevice::_destroySwapchain(const VulkanSwapchain *swapchain) {
        for (uint32_t i = 0; i < swapchain->resolveImages.size(); i++) {
            destroyImage(swapchain->colorTargets[i]);
            destroyImage(swapchain->depthTargets[i]);
            vkDestroyImageView(device, swapchain->resolveImageViews[i], nullptr);
        }
        vkDestroySwapchainKHR(device, swapchain->swapchain, nullptr);
    }

    void VulkanRenderingDevice::destroySwapchain(Swapchain *swapchain) {
        DEBUG_ASSERT(swapchain != nullptr);

        const auto vkSwapchain = dynamic_cast<VulkanSwapchain *>(swapchain);
        _destroySwapchain(vkSwapchain);
        delete vkSwapchain;
    }

    uint32_t VulkanRenderingDevice::getQueueFamily(const QueueFamilyFlags queueFamilyFlags, Surface *surface) {
        VkQueueFlags pickedQueueFlags = VK_QUEUE_FLAG_BITS_MAX_ENUM;
        uint32_t pickedQueueFamilyIndex = std::numeric_limits<uint32_t>::max();

        for (uint32_t i = 0; i < queueFamilies.size(); i++) {
            if (queueFamilies[i].empty())
                continue;

            if (surface != nullptr && !renderingContext->supportsPresent(
                    physicalDevice.device, i, dynamic_cast<VulkanSurface *>(surface)))
                continue;

            const VkQueueFlags optionQueueFlags = physicalDevice.queueFamilies[i].properties.queueFlags;
            const bool includesAllBits = static_cast<QueueFamilyFlags>(optionQueueFlags) & queueFamilyFlags;
            if (const bool preferLessBits = optionQueueFlags < pickedQueueFlags;
                includesAllBits && preferLessBits) {
                pickedQueueFamilyIndex = i;
                pickedQueueFlags = optionQueueFlags;
            }
        }

        if (pickedQueueFamilyIndex > queueFamilies.size())
            error<CantCreateError>("Failed to find suitable queue family");

        return pickedQueueFamilyIndex;
    }

    Fence *VulkanRenderingDevice::createFence() {
        const auto o = new VulkanFence();

        constexpr VkFenceCreateInfo fenceInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0
        };

        if (vkCreateFence(device, &fenceInfo, nullptr, &o->fence) != VK_SUCCESS)
            error<CantCreateError>("Call to vkCreateFence failed.");

        return o;
    }

    void VulkanRenderingDevice::waitOnFence(const Fence *fence) {
        const auto o = dynamic_cast<const VulkanFence *>(fence);
        vkWaitForFences(device, 1, &o->fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
    }

    void VulkanRenderingDevice::destroyFence(Fence *fence) {
        const auto o = dynamic_cast<VulkanFence *>(fence);
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
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &o->semaphore) != VK_SUCCESS)
            error<CantCreateError>("Call to vkCreateSemaphore failed.");

        return o;
    }

    void VulkanRenderingDevice::destroySemaphore(Semaphore *semaphore) {
        const auto o = dynamic_cast<VulkanSemaphore *>(semaphore);
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
        if (vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool) != VK_SUCCESS)
            error<CantCreateError>("Call to vkCreateCommandPool failed.");
        const auto o = new VulkanCommandPool{};
        o->pool = commandPool;
        o->type = type;
        o->queueFamily = queueFamily;
        return o;
    }

    void VulkanRenderingDevice::resetCommandPool(CommandPool *pool) {
        const auto *o = dynamic_cast<VulkanCommandPool *>(pool);
        if (vkResetCommandPool(device, o->pool, 0) != VK_SUCCESS)
            error<CantCreateError>("Call to vkResetCommandPool failed.");
    }

    void VulkanRenderingDevice::destroyCommandPool(CommandPool *pool) {
        const auto *o = dynamic_cast<VulkanCommandPool *>(pool);
        vkDestroyCommandPool(device, o->pool, nullptr);
        delete o;
    }

    CommandBuffer *VulkanRenderingDevice::createCommandBuffer(CommandPool *pool) {
        const auto *p = dynamic_cast<VulkanCommandPool *>(pool);

        const VkCommandBufferAllocateInfo commandBufferInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = p->pool,
            .level = static_cast<VkCommandBufferLevel>(p->type),
            .commandBufferCount = 1
        };

        VkCommandBuffer commandBuffer;
        if (vkAllocateCommandBuffers(device, &commandBufferInfo, &commandBuffer) != VK_SUCCESS)
            error<CantCreateError>("Call to vkAllocateCommandBuffers failed.");
        const auto o = new VulkanCommandBuffer{};
        o->commandBuffer = commandBuffer;

        return o;
    }

    void VulkanRenderingDevice::beginCommandBuffer(CommandBuffer *commandBuffer) {
        const auto o = dynamic_cast<VulkanCommandBuffer *>(commandBuffer);

        constexpr VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr
        };
        if (vkBeginCommandBuffer(o->commandBuffer, &beginInfo) != VK_SUCCESS)
            error<CantCreateError>("Call to vkBeginCommandBuffer failed.");
    }

    void VulkanRenderingDevice::endCommandBuffer(CommandBuffer *commandBuffer) {
        const auto o = dynamic_cast<VulkanCommandBuffer *>(commandBuffer);
        vkEndCommandBuffer(o->commandBuffer);
    }

    CommandQueue *VulkanRenderingDevice::createCommandQueue() {
        auto commandQueue = new VulkanCommandQueue();

        // TODO

        return commandQueue;
    }

    void VulkanRenderingDevice::executeCommandQueueAndPresent(
        CommandQueue *commandQueue,
        const std::vector<Semaphore *> &waitSemaphores,
        const std::vector<CommandBuffer *> &commandBuffers,
        const std::vector<Semaphore *> &semaphores,
        Fence *fence,
        const std::vector<Swapchain *> &swapchains
    ) {
        const auto vkCommandQueue = dynamic_cast<VulkanCommandQueue *>(commandQueue);
        const Queue queue = queueFamilies[vkCommandQueue->queueFamilyIndex][vkCommandQueue->queueIndex];
        const auto vkFence = dynamic_cast<VulkanFence *>(fence);

        std::vector<VkSemaphore> vkWaitSemaphores{};
        vkWaitSemaphores.reserve(waitSemaphores.size());
        std::vector<VkPipelineStageFlags> vkWaitStages{};
        vkWaitSemaphores.reserve(vkWaitSemaphores.size());
        for (const auto &semaphore: waitSemaphores) {
            vkWaitSemaphores.push_back(dynamic_cast<VulkanSemaphore *>(semaphore)->semaphore);
            vkWaitStages.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        }

        if (!commandBuffers.empty()) {
            std::vector<VkCommandBuffer> vkCommandBuffers{};
            vkCommandBuffers.reserve(commandBuffers.size());
            for (const auto &commandBuffer: commandBuffers)
                vkCommandBuffers.push_back(dynamic_cast<VulkanCommandBuffer *>(commandBuffer)->commandBuffer);

            std::vector<VkSemaphore> signalSemaphores{};
            signalSemaphores.reserve(semaphores.size());
            for (const auto &semaphore: semaphores)
                signalSemaphores.push_back(dynamic_cast<VulkanSemaphore *>(semaphore)->semaphore);

            const VkSubmitInfo submitInfo{
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .pNext = nullptr,
                .waitSemaphoreCount = static_cast<uint32_t>(vkWaitSemaphores.size()),
                .pWaitSemaphores = vkWaitSemaphores.data(),
                .pWaitDstStageMask = vkWaitStages.data(),
                .commandBufferCount = static_cast<uint32_t>(vkCommandBuffers.size()),
                .pCommandBuffers = vkCommandBuffers.data(),
                .signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size()),
                .pSignalSemaphores = signalSemaphores.data()
            };

            const auto submitResult = vkQueueSubmit(
                queue.queue,
                1,
                &submitInfo,
                vkFence != nullptr ? vkFence->fence : VK_NULL_HANDLE
            );
            if (submitResult == VK_ERROR_DEVICE_LOST) {
                // TODO: Print crash log
                CRASH("Vulkan device lost");
            }
            if (submitResult != VK_SUCCESS)
                error<CantCreateError>("Call to vkQueueSubmit failed.");
        }

        if (!swapchains.empty()) {
            std::vector<VkSwapchainKHR> vkSwapchains{};
            vkSwapchains.reserve(swapchains.size());
            std::vector<uint32_t> imageIndices{};
            imageIndices.reserve(swapchains.size());

            for (const auto &swapchain: swapchains) {
                const auto vkSwapchain = dynamic_cast<VulkanSwapchain *>(swapchain);
                vkSwapchains.push_back(vkSwapchain->swapchain);
                // TODO: Need to vkAcquireNextImageKHR before doing this
                DEBUG_ASSERT(vkSwapchain->imageIndex < vkSwapchain->colorTargets.size());
                imageIndices.push_back(vkSwapchain->imageIndex);

                // TODO: Bad.
                auto pool = createCommandPool(vkCommandQueue->queueFamilyIndex, CommandBufferType::Primary);
                CommandBuffer *commandBuffer = createCommandBuffer(pool);
                auto vkCommandBuffer = dynamic_cast<VulkanCommandBuffer *>(commandBuffer)->commandBuffer;

                beginCommandBuffer(commandBuffer);
                VkImageMemoryBarrier barrier{
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .pNext = nullptr,
                    .srcAccessMask = VK_ACCESS_NONE,
                    .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = vkSwapchain->resolveImages[vkSwapchain->imageIndex],
                    .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1
                    }
                };

                vkCmdPipelineBarrier(
                    vkCommandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    1,
                    &barrier
                );

                const VkImageCopy copy{
                    .srcSubresource = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel = 0,
                        .baseArrayLayer = 0,
                        .layerCount = 1
                    },
                    .srcOffset = {
                        .x = 0,
                        .y = 0,
                        .z = 0
                    },
                    .dstSubresource = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel = 0,
                        .baseArrayLayer = 0,
                        .layerCount = 1
                    },
                    .dstOffset = {
                        .x = 0,
                        .y = 0,
                        .z = 0
                    },
                    .extent = {
                        .width = 1280, // TODO
                        .height = 720, // TODO
                        .depth = 1
                    }
                };

                vkCmdCopyImage(
                    vkCommandBuffer,
                    vkSwapchain->colorTargets[vkSwapchain->imageIndex]->image,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    vkSwapchain->resolveImages[vkSwapchain->imageIndex],
                    barrier.newLayout,
                    1,
                    &copy
                );

                barrier.oldLayout = barrier.newLayout;
                barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                barrier.srcAccessMask = VK_ACCESS_NONE;
                barrier.dstAccessMask = VK_ACCESS_NONE;

                vkCmdPipelineBarrier(
                    vkCommandBuffer,
                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                    0,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    1,
                    &barrier
                );

                endCommandBuffer(commandBuffer);
                executeCommandQueueAndPresent(commandQueue, {}, {commandBuffer}, {}, nullptr, {});

                // TODO: Bad.
                vkDeviceWaitIdle(device);
                destroyCommandPool(pool);
            }

            const VkPresentInfoKHR presentInfo{
                .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .pNext = nullptr,
                .waitSemaphoreCount = 0,
                .pWaitSemaphores = nullptr,
                .swapchainCount = static_cast<uint32_t>(vkSwapchains.size()),
                .pSwapchains = vkSwapchains.data(),
                .pImageIndices = imageIndices.data(),
                .pResults = nullptr // TODO: Check results for every swapchain
            };

            // TODO: Handle suboptimal and out of date errors?
            if (vkQueuePresentKHR(queue.queue, &presentInfo) != VK_SUCCESS)
                error<CantCreateError>("Call to vkQueuePresentKHR failed.");
        }
    }

    void VulkanRenderingDevice::destroyCommandQueue(CommandQueue *commandQueue) {
        const auto o = dynamic_cast<VulkanCommandQueue *>(commandQueue);

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
        if (vmaCreateBuffer(
                allocator,
                &bufferCreateInfo,
                &allocationCreateInfo,
                &buffer,
                &allocation,
                &allocationInfo
            ) != VK_SUCCESS)
            error<CantCreateError>("Failed to create buffer");

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
        if (vmaCreateImage(
                allocator,
                &imageCreateInfo,
                &allocationCreateInfo,
                &image,
                &allocation,
                nullptr
            ) != VK_SUCCESS)
            error<CantCreateError>("Failed to create image");

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

        if (const auto e = vkCreateImageView(device, &imageViewInfo, nullptr, &imageView);
            e != VK_SUCCESS) {
            vmaDestroyImage(allocator, image, allocation);
            error<CantCreateError>("Call to vkCreateImageView failed.");
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
        const auto o = dynamic_cast<VulkanImage *>(image);
        std::byte *data;
        vmaMapMemory(allocator, o->allocation, std::bit_cast<void **>(&data));
        return data;
    }

    void VulkanRenderingDevice::unmapImage(Image *image) {
        const auto o = dynamic_cast<VulkanImage *>(image);
        vmaUnmapMemory(allocator, o->allocation);
    }

    void VulkanRenderingDevice::destroyImage(Image *image) {
        const auto o = dynamic_cast<VulkanImage *>(image);
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
        if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
            error<CantCreateError>("Call to vkCreateSampler failed.");

        const auto o = new VulkanSampler{};
        o->state = state;
        o->sampler = sampler;
        return o;
    }

    void VulkanRenderingDevice::destroySampler(Sampler *sampler) {
        const auto o = dynamic_cast<VulkanSampler *>(sampler);
        vkDestroySampler(device, o->sampler, nullptr);
        delete o;
    }

    Shader *VulkanRenderingDevice::createShaderFromSpirv(const std::string &name,
                                                         const std::vector<ShaderStageData> &stages) {
        const auto o = new VulkanShader();
        if (!reflectShader(stages, o)) {
            delete o;
            error<CantCreateError>("Shader reflection failed.");
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
                .pCode = std::bit_cast<const uint32_t *>(spirv.data())
            };

            VkShaderModule module;
            if (vkCreateShaderModule(device, &shaderModuleInfo, nullptr, &module) != VK_SUCCESS)
                error<CantCreateError>("Call to vkCreateShaderModule failed.");

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
            if (vkCreateDescriptorSetLayout(device, &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout) !=
                VK_SUCCESS)
                error<CantCreateError>("Call to vkCreateDescriptorSetLayout failed.");
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
        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
            error<CantCreateError>("Call to vkCreatePipelineLayout failed.");
        o->pipelineLayout = pipelineLayout;

        return o;
    }

    void VulkanRenderingDevice::destroyShaderModules(Shader *shader) {
        const auto o = dynamic_cast<VulkanShader *>(shader);
        for (const auto &stageInfo: o->shaderStageInfos)
            vkDestroyShaderModule(device, stageInfo.module, nullptr);
        o->shaderStageInfos.clear();
    }

    void VulkanRenderingDevice::destroyShader(Shader *shader) {
        const auto o = dynamic_cast<VulkanShader *>(shader);

        destroyShaderModules(o);
        for (const auto &descriptorSetLayout: o->descriptorSetLayouts)
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        vkDestroyPipelineLayout(device, o->pipelineLayout, nullptr);

        delete o;
    }

    GraphicsCard VulkanRenderingDevice::getPhysicalDevice() const {
        return physicalDevice;
    }

    VkImageSubresourceLayers VulkanRenderingDevice::_imageSubresourceLayers(const ImageSubresourceLayers &layers) {
        return {
            .aspectMask = toVkImageAspectFlags(layers.aspect),
            .mipLevel = layers.mipmap,
            .baseArrayLayer = layers.baseLayer,
            .layerCount = layers.layerCount
        };
    }

    VkBufferImageCopy VulkanRenderingDevice::_bufferImageCopyRegion(const BufferImageCopyRegion &region) {
        return {
            .bufferOffset = region.bufferOffset,
            .bufferRowLength = {},
            .bufferImageHeight = {},
            .imageSubresource = _imageSubresourceLayers(region.imageSubresourceLayers),
            .imageOffset = {
                .x = region.imageOffset.x,
                .y = region.imageOffset.y,
                .z = region.imageOffset.z,
            },
            .imageExtent = {
                .width = region.imageRegionSize.x,
                .height = region.imageRegionSize.y,
                .depth = region.imageRegionSize.z,
            }
        };
    }

    void VulkanRenderingDevice::commandPipelineBarrier(
        CommandBuffer *commandBuffer,
        const PipelineStageFlags sourceStages,
        const PipelineStageFlags destinationStages,
        const std::vector<MemoryBarrier> &memoryBarriers,
        const std::vector<BufferBarrier> &bufferBarriers,
        const std::vector<ImageBarrier> &imageBarriers
    ) {
        if (!enabledFeatures.dynamicRendering) {
            std::vector<VkMemoryBarrier> vkMemoryBarriers{};
            vkMemoryBarriers.reserve(memoryBarriers.size());
            for (const auto &[sourceAccess, targetAccess]: memoryBarriers) {
                vkMemoryBarriers.push_back({
                    .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
                    .pNext = nullptr,
                    .srcAccessMask = toVkAccessFlags(sourceAccess),
                    .dstAccessMask = toVkAccessFlags(targetAccess)
                });
            }

            std::vector<VkBufferMemoryBarrier> vkBufferBarriers{};
            vkBufferBarriers.reserve(bufferBarriers.size());
            for (const auto &[buffer, sourceAccess, destinationAccess, offset, size]: bufferBarriers) {
                vkBufferBarriers.push_back({
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                    .pNext = nullptr,
                    .srcAccessMask = toVkAccessFlags(sourceAccess),
                    .dstAccessMask = toVkAccessFlags(destinationAccess),
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .buffer = dynamic_cast<VulkanBuffer *>(buffer)->buffer,
                    .offset = offset,
                    .size = size
                });
            }

            std::vector<VkImageMemoryBarrier> vkImageBarriers{};
            vkImageBarriers.reserve(imageBarriers.size());
            for (const auto &[image, sourceAccess, destinationAccess, oldLayout, newLayout, subresources]:
                 imageBarriers) {
                vkImageBarriers.push_back({
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .pNext = nullptr,
                    .srcAccessMask = toVkAccessFlags(sourceAccess),
                    .dstAccessMask = toVkAccessFlags(destinationAccess),
                    .oldLayout = toVkImageLayout(oldLayout),
                    .newLayout = toVkImageLayout(newLayout),
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = dynamic_cast<VulkanImage *>(image)->image,
                    .subresourceRange = {
                        .aspectMask = toVkImageAspectFlags(subresources.aspect),
                        .baseMipLevel = subresources.baseMipmap,
                        .levelCount = subresources.mipmapCount,
                        .baseArrayLayer = subresources.baseLayer,
                        .layerCount = subresources.layerCount
                    }
                });
            }

            vkCmdPipelineBarrier(
                dynamic_cast<VulkanCommandBuffer *>(commandBuffer)->commandBuffer,
                toVkPipelineStages(sourceStages),
                toVkPipelineStages(destinationStages),
                0,
                static_cast<uint32_t>(vkMemoryBarriers.size()),
                vkMemoryBarriers.data(),
                static_cast<uint32_t>(vkBufferBarriers.size()),
                vkBufferBarriers.data(),
                static_cast<uint32_t>(vkImageBarriers.size()),
                vkImageBarriers.data()
            );
        } else {
            std::vector<VkMemoryBarrier2> vkMemoryBarriers{};
            vkMemoryBarriers.reserve(memoryBarriers.size());
            for (const auto &[sourceAccess, targetAccess]: memoryBarriers) {
                vkMemoryBarriers.push_back({
                    .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
                    .pNext = nullptr,
                    .srcAccessMask = toVkAccessFlags(sourceAccess),
                    .dstAccessMask = toVkAccessFlags(targetAccess)
                });
            }

            std::vector<VkBufferMemoryBarrier2> vkBufferBarriers{};
            vkBufferBarriers.reserve(bufferBarriers.size());
            for (const auto &[buffer, sourceAccess, destinationAccess, offset, size]: bufferBarriers) {
                vkBufferBarriers.push_back({
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                    .pNext = nullptr,
                    .srcAccessMask = toVkAccessFlags(sourceAccess),
                    .dstAccessMask = toVkAccessFlags(destinationAccess),
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .buffer = dynamic_cast<VulkanBuffer *>(buffer)->buffer,
                    .offset = offset,
                    .size = size
                });
            }

            std::vector<VkImageMemoryBarrier2> vkImageBarriers{};
            vkImageBarriers.reserve(imageBarriers.size());
            for (const auto &[image, sourceAccess, destinationAccess, oldLayout, newLayout, subresources]:
                 imageBarriers) {
                vkImageBarriers.push_back({
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .pNext = nullptr,
                    .srcAccessMask = toVkAccessFlags(sourceAccess),
                    .dstAccessMask = toVkAccessFlags(destinationAccess),
                    .oldLayout = toVkImageLayout(oldLayout),
                    .newLayout = toVkImageLayout(newLayout),
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = dynamic_cast<VulkanImage *>(image)->image,
                    .subresourceRange = {
                        .aspectMask = toVkImageAspectFlags(subresources.aspect),
                        .baseMipLevel = subresources.baseMipmap,
                        .levelCount = subresources.mipmapCount,
                        .baseArrayLayer = subresources.baseLayer,
                        .layerCount = subresources.layerCount
                    }
                });
            }

            const VkDependencyInfo dependencyInfo{
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .pNext = nullptr,
                .dependencyFlags = 0,
                .memoryBarrierCount = static_cast<uint32_t>(vkMemoryBarriers.size()),
                .pMemoryBarriers = vkMemoryBarriers.data(),
                .bufferMemoryBarrierCount = static_cast<uint32_t>(vkBufferBarriers.size()),
                .pBufferMemoryBarriers = vkBufferBarriers.data(),
                .imageMemoryBarrierCount = static_cast<uint32_t>(vkImageBarriers.size()),
                .pImageMemoryBarriers = vkImageBarriers.data()
            };

            vkCmdPipelineBarrier2(
                dynamic_cast<VulkanCommandBuffer *>(commandBuffer)->commandBuffer,
                &dependencyInfo
            );
        }
    }

    void VulkanRenderingDevice::commandClearBuffer(CommandBuffer *commandBuffer, Buffer *buffer, const uint64_t offset,
                                                   const uint64_t size) {
        vkCmdFillBuffer(
            dynamic_cast<VulkanCommandBuffer *>(commandBuffer)->commandBuffer,
            dynamic_cast<VulkanBuffer *>(buffer)->buffer,
            offset,
            size,
            0
        );
    }

    void VulkanRenderingDevice::commandCopyBuffer(CommandBuffer *commandBuffer, Buffer *source, Buffer *destination,
                                                  const std::vector<BufferCopyRegion> &regions) {
        std::vector<VkBufferCopy> vkRegions{};
        vkRegions.reserve(regions.size());
        for (const auto &[sourceOffset, destinationOffset, size]: regions) {
            vkRegions.push_back({
                .srcOffset = sourceOffset,
                .dstOffset = destinationOffset,
                .size = size
            });
        }

        vkCmdCopyBuffer(
            dynamic_cast<VulkanCommandBuffer *>(commandBuffer)->commandBuffer,
            dynamic_cast<VulkanBuffer *>(source)->buffer,
            dynamic_cast<VulkanBuffer *>(destination)->buffer,
            vkRegions.size(),
            vkRegions.data()
        );
    }

    void VulkanRenderingDevice::commandCopyImage(CommandBuffer *commandBuffer, Image *source, ImageLayout sourceLayout,
                                                 Image *destination, ImageLayout destinationLayout,
                                                 const std::vector<ImageCopyRegion> &regions) {
        std::vector<VkImageCopy> vkRegions{};
        vkRegions.reserve(regions.size());
        for (const auto &[sourceSubresources, sourceOffset, destinationSubresources, destinationOffset, size]:
             regions) {
            vkRegions.push_back({
                .srcSubresource = _imageSubresourceLayers(sourceSubresources),
                .srcOffset = {
                    .x = sourceOffset.x,
                    .y = sourceOffset.y,
                    .z = sourceOffset.z
                },
                .dstSubresource = _imageSubresourceLayers(destinationSubresources),
                .dstOffset = {
                    .x = destinationOffset.x,
                    .y = destinationOffset.y,
                    .z = destinationOffset.z
                },
                .extent = {
                    .width = size.x,
                    .height = size.y,
                    .depth = size.z
                }
            });
        }

        vkCmdCopyImage(
            dynamic_cast<VulkanCommandBuffer *>(commandBuffer)->commandBuffer,
            dynamic_cast<VulkanImage *>(source)->image,
            static_cast<VkImageLayout>(sourceLayout),
            dynamic_cast<VulkanImage *>(destination)->image,
            static_cast<VkImageLayout>(destinationLayout),
            vkRegions.size(),
            vkRegions.data()
        );
    }

    void VulkanRenderingDevice::commandResolveImage(CommandBuffer *commandBuffer, Image *source,
                                                    ImageLayout sourceLayout, uint32_t sourceLayer,
                                                    uint32_t sourceMipmap, Image *destination,
                                                    ImageLayout destinationLayout, uint32_t destinationLayer,
                                                    uint32_t destinationMipmap) {
        // TODO
    }

    void VulkanRenderingDevice::commandClearColorImage(CommandBuffer *commandBuffer, Image *image,
                                                       ImageLayout imageLayout, const glm::vec4 &color,
                                                       const ImageSubresourceRange &subresource) {
        const VkClearColorValue vkColor = {
            color.r,
            color.b,
            color.g,
            color.a
        };
        const VkImageSubresourceRange vkSubresource{
            .aspectMask = toVkImageAspectFlags(subresource.aspect),
            .baseMipLevel = subresource.baseMipmap,
            .levelCount = subresource.mipmapCount,
            .baseArrayLayer = subresource.baseLayer,
            .layerCount = subresource.layerCount
        };

        vkCmdClearColorImage(
            dynamic_cast<VulkanCommandBuffer *>(commandBuffer)->commandBuffer,
            dynamic_cast<VulkanImage *>(image)->image,
            static_cast<VkImageLayout>(imageLayout),
            &vkColor,
            1,
            &vkSubresource
        );
    }

    void VulkanRenderingDevice::commandCopyBufferToImage(CommandBuffer *commandBuffer, Buffer *buffer, Image *image,
                                                         ImageLayout layout,
                                                         const std::vector<BufferImageCopyRegion> &regions) {
        std::vector<VkBufferImageCopy> vkRegions{};
        vkRegions.reserve(regions.size());
        for (const auto &region: regions)
            vkRegions.push_back(_bufferImageCopyRegion(region));

        vkCmdCopyBufferToImage(
            dynamic_cast<VulkanCommandBuffer *>(commandBuffer)->commandBuffer,
            dynamic_cast<VulkanBuffer *>(buffer)->buffer,
            dynamic_cast<VulkanImage *>(image)->image,
            static_cast<VkImageLayout>(layout),
            vkRegions.size(),
            vkRegions.data()
        );
    }

    void VulkanRenderingDevice::commandCopyImageToBuffer(CommandBuffer *commandBuffer, Image *image, ImageLayout layout,
                                                         Buffer *buffer,
                                                         const std::vector<BufferImageCopyRegion> &regions) {
        std::vector<VkBufferImageCopy> vkRegions{};
        vkRegions.reserve(regions.size());
        for (const auto &region: regions)
            vkRegions.push_back(_bufferImageCopyRegion(region));

        vkCmdCopyImageToBuffer(
            dynamic_cast<VulkanCommandBuffer *>(commandBuffer)->commandBuffer,
            dynamic_cast<VulkanImage *>(image)->image,
            static_cast<VkImageLayout>(layout),
            dynamic_cast<VulkanBuffer *>(buffer)->buffer,
            vkRegions.size(),
            vkRegions.data()
        );
    }

    void VulkanRenderingDevice::commandBeginLabel(CommandBuffer *commandBuffer, const std::string &label,
                                                  const glm::vec3 &color) {
        // TODO
    }

    void VulkanRenderingDevice::commandEndLabel(CommandBuffer *commandBuffer) {
        // TODO
    }
}
