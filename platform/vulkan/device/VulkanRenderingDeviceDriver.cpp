#define VMA_IMPLEMENTATION

#include "VulkanRenderingDeviceDriver.h"

#include <algorithm>
#include <array>
#include <format>
#include <experimental/scope>
#include <map>
#include <new>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <vk_mem_alloc.h>
#include "platform/vulkan/Vulkan.h"

#include "core/rendering/AttachmentInfo.h"
#include "platform/vulkan/context/VulkanRenderingContextDriver.h"
#include "platform/vulkan/presentation/VulkanSwapchain.h"
#include "core/buffer/BufferImageCopyRegion.h"
#include "core/buffer/BufferCopyRegion.h"
#include "platform/vulkan/buffer/VulkanBuffer.h"
#include "platform/vulkan/command/VulkanCommandBuffer.h"
#include "platform/vulkan/command/VulkanCommandPool.h"
#include "platform/vulkan/command/VulkanCommandQueue.h"
#include "platform/vulkan/command/VulkanFence.h"
#include "platform/vulkan/command/VulkanSemaphore.h"
#include "core/error/CantCreateError.h"
#include "core/error/Macros.h"
#include "core/error/Shader.h"
#include "core/error/SwapchainError.h"
#include "platform/vulkan/image/VulkanImage.h"
#include "platform/vulkan/image/VulkanSampler.h"
#include "core/image/ImageCopyRegion.h"
#include "pipeline/VulkanComputePipeline.h"
#include "pipeline/VulkanGraphicsPipeline.h"
#include "pipeline/VulkanPipelineLayout.h"
#include "platform/vulkan/shader/VulkanShader.h"

namespace Vixen {
    namespace {
        [[nodiscard]] auto resourceCreationErrorCode(
            const VkResult result,
            const ResourceCreationErrorCode fallback
        ) noexcept -> ResourceCreationErrorCode {
            if (result == VK_ERROR_OUT_OF_HOST_MEMORY)
                return ResourceCreationErrorCode::OutOfHostMemory;

            if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY)
                return ResourceCreationErrorCode::OutOfDeviceMemory;

            if (result == VK_ERROR_FORMAT_NOT_SUPPORTED)
                return ResourceCreationErrorCode::UnsupportedFormat;

            if (result == VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR ||
                result == VK_ERROR_FEATURE_NOT_PRESENT ||
                result == VK_ERROR_NOT_PERMITTED)
                return ResourceCreationErrorCode::UnsupportedUsage;

            if (result == VK_ERROR_TOO_MANY_OBJECTS ||
                result == VK_ERROR_OUT_OF_POOL_MEMORY ||
                result == VK_ERROR_FRAGMENTED_POOL ||
                result == VK_ERROR_FRAGMENTATION ||
                result == VK_ERROR_COMPRESSION_EXHAUSTED_EXT ||
                result == VK_ERROR_NOT_ENOUGH_SPACE_KHR)
                return ResourceCreationErrorCode::ExceedsDeviceLimits;

            if (result == VK_ERROR_VALIDATION_FAILED)
                return ResourceCreationErrorCode::InvalidDescription;

            return fallback;
        }

        [[nodiscard]] auto makeResourceCreationError(
            const VkResult result,
            const std::string_view operation,
            const ResourceCreationErrorCode fallback = ResourceCreationErrorCode::NativeObjectCreationFailed
        ) -> ResourceCreationError {
            const auto resultName = std::string{string_VkResult(result)};

            return ResourceCreationError{
                .code = resourceCreationErrorCode(result, fallback),
                .message = std::format(
                    "{} failed with {} ({})",
                    operation,
                    resultName,
                    static_cast<int32_t>(result)
                ),
                .nativeError = NativeResourceCreationError{
                    .backend = "Vulkan",
                    .operation = std::string{operation},
                    .code = static_cast<int64_t>(result),
                    .name = resultName
                }
            };
        }

        template <typename Value>
        [[nodiscard]] auto requireVkConversion(
            std::expected<Value, ResourceCreationError> conversion
        ) -> Value {
            if (!conversion)
                throw std::invalid_argument{conversion.error().message};

            return *conversion;
        }
    }

    auto VulkanRenderingDeviceDriver::initializeExtensions() -> std::expected<void, Error> {
        std::map<std::string, bool> requestedExtensions;

        requestedExtensions[VK_KHR_SWAPCHAIN_EXTENSION_NAME] = true;
        requestedExtensions[VK_KHR_MAINTENANCE_2_EXTENSION_NAME] = false;

        #ifdef DEBUG_ENABLED
        requestedExtensions[VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME] = false;
        requestedExtensions[VK_EXT_DEVICE_FAULT_EXTENSION_NAME] = false;
        requestedExtensions[VK_EXT_DEBUG_MARKER_EXTENSION_NAME] = false;
        #endif

        uint32_t extensionCount = 0;
        if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr) !=
            VK_SUCCESS)
            error<CantCreateError>("Call to vkEnumerateDeviceExtensionProperties failed.");
        std::vector<VkExtensionProperties> availableExtensions{extensionCount};
        if (vkEnumerateDeviceExtensionProperties(
            physicalDevice,
            nullptr,
            &extensionCount,
            availableExtensions.
            data()
        ) != VK_SUCCESS)
            error<CantCreateError>("Call to vkEnumerateDeviceExtensionProperties failed.");

        spdlog::info(
            "Using device {}\n"
            "    * Supported device extensions\n"
            "{}",
            physicalDeviceProperties.deviceName,
            std::ranges::fold_left_first(
                availableExtensions |
                std::views::transform(
                    [](
                    const auto& extension
                ) {
                        return std::format("        - {}", extension.extensionName);
                    }
                ),
                [](
                const auto& a,
                const auto& b
            ) {
                    return a + "\n" + b;
                }
            ).value_or("")
        );
        for (uint32_t i = 0; i < extensionCount; i++) {
            if (const auto& extensionName = availableExtensions[i].extensionName;
                requestedExtensions.contains(extensionName))
                enabledExtensionNames.emplace_back(extensionName);
        }

        for (const auto& [extensionName, required] : requestedExtensions) {
            if (std::ranges::find(enabledExtensionNames.begin(), enabledExtensionNames.end(), extensionName) ==
                enabledExtensionNames.end()) {
                if (required)
                    return std::unexpected(Error::InitializationFailed);

                spdlog::debug("Optional extension {} was not found.", extensionName);
            }
        }

        return {};
    }

    void VulkanRenderingDeviceDriver::checkFeatures() const {
        if (physicalDeviceFeatures.vulkan12.timelineSemaphore != VK_TRUE)
            error<CantCreateError>("Device lacks timeline semaphore extension support.");

        if (physicalDeviceFeatures.vulkan13.dynamicRendering != VK_TRUE)
            error<CantCreateError>("Device lacks dynamic rendering extension support.");

        if (physicalDeviceFeatures.vulkan13.synchronization2 != VK_TRUE)
            error<CantCreateError>("Device lacks synchronization 2 extension support");

        if (physicalDeviceFeatures.core.features.imageCubeArray != VK_TRUE)
            error<CantCreateError>("Device lacks image cube array feature.");

        if (physicalDeviceFeatures.core.features.independentBlend != VK_TRUE)
            error<CantCreateError>("Device lacks independent blend feature.");
    }

    void VulkanRenderingDeviceDriver::checkCapabilities() {
        auto isAvailable = [&](const std::string& extension) constexpr -> bool {
            return std::ranges::find(enabledExtensionNames, extension) != enabledExtensionNames.end();
        };

        if (isAvailable(VK_EXT_DEVICE_FAULT_EXTENSION_NAME))
            enabledFeatures.deviceFault = true;
    }

    auto VulkanRenderingDeviceDriver::initializeDevice() -> std::expected<void, Error> {
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
        static constexpr float queuePriorities[1] = {0.0f};
        for (uint32_t i = 0; i < queueFamilyProperties.size(); i++) {
            if ((queueFamilyProperties[i].queueFlags & (
                VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT)) == 0)
                continue;

            queueCreateInfos.push_back(
                {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .queueFamilyIndex = i,
                    .queueCount = std::min(queueFamilyProperties[i].queueCount, static_cast<uint32_t>(1)),
                    .pQueuePriorities = queuePriorities
                }
            );
        }

        VkPhysicalDeviceFaultFeaturesEXT faultFeatures{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT,
            .pNext = nullptr,
            .deviceFault = enabledFeatures.deviceFault,
            .deviceFaultVendorBinary = VK_FALSE
        };

        VkPhysicalDeviceVulkan12Features enabled12{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
            .pNext = nullptr,
            .samplerMirrorClampToEdge = VK_FALSE,
            .drawIndirectCount = VK_FALSE,
            .storageBuffer8BitAccess = VK_FALSE,
            .uniformAndStorageBuffer8BitAccess = VK_FALSE,
            .storagePushConstant8 = VK_FALSE,
            .shaderBufferInt64Atomics = VK_FALSE,
            .shaderSharedInt64Atomics = VK_FALSE,
            .shaderFloat16 = VK_FALSE,
            .shaderInt8 = VK_FALSE,
            .descriptorIndexing = VK_FALSE,
            .shaderInputAttachmentArrayDynamicIndexing = VK_FALSE,
            .shaderUniformTexelBufferArrayDynamicIndexing = VK_FALSE,
            .shaderStorageTexelBufferArrayDynamicIndexing = VK_FALSE,
            .shaderUniformBufferArrayNonUniformIndexing = VK_FALSE,
            .shaderSampledImageArrayNonUniformIndexing = VK_FALSE,
            .shaderStorageBufferArrayNonUniformIndexing = VK_FALSE,
            .shaderStorageImageArrayNonUniformIndexing = VK_FALSE,
            .shaderInputAttachmentArrayNonUniformIndexing = VK_FALSE,
            .shaderUniformTexelBufferArrayNonUniformIndexing = VK_FALSE,
            .shaderStorageTexelBufferArrayNonUniformIndexing = VK_FALSE,
            .descriptorBindingUniformBufferUpdateAfterBind = VK_FALSE,
            .descriptorBindingSampledImageUpdateAfterBind = VK_FALSE,
            .descriptorBindingStorageImageUpdateAfterBind = VK_FALSE,
            .descriptorBindingStorageBufferUpdateAfterBind = VK_FALSE,
            .descriptorBindingUniformTexelBufferUpdateAfterBind = VK_FALSE,
            .descriptorBindingStorageTexelBufferUpdateAfterBind = VK_FALSE,
            .descriptorBindingUpdateUnusedWhilePending = VK_FALSE,
            .descriptorBindingPartiallyBound = VK_FALSE,
            .descriptorBindingVariableDescriptorCount = VK_FALSE,
            .runtimeDescriptorArray = VK_FALSE,
            .samplerFilterMinmax = VK_FALSE,
            .scalarBlockLayout = VK_FALSE,
            .imagelessFramebuffer = VK_FALSE,
            .uniformBufferStandardLayout = VK_FALSE,
            .shaderSubgroupExtendedTypes = VK_FALSE,
            .separateDepthStencilLayouts = VK_FALSE,
            .hostQueryReset = VK_FALSE,
            .timelineSemaphore = VK_TRUE,
            .bufferDeviceAddress = VK_FALSE,
            .bufferDeviceAddressCaptureReplay = VK_FALSE,
            .bufferDeviceAddressMultiDevice = VK_FALSE,
            .vulkanMemoryModel = VK_FALSE,
            .vulkanMemoryModelDeviceScope = VK_FALSE,
            .vulkanMemoryModelAvailabilityVisibilityChains = VK_FALSE,
            .shaderOutputViewportIndex = VK_FALSE,
            .shaderOutputLayer = VK_FALSE,
            .subgroupBroadcastDynamicId = VK_FALSE
        };
        faultFeatures.pNext = &enabled12;

        VkPhysicalDeviceVulkan13Features enabled13{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .pNext = nullptr,
            .robustImageAccess = VK_FALSE,
            .inlineUniformBlock = VK_FALSE,
            .descriptorBindingInlineUniformBlockUpdateAfterBind = VK_FALSE,
            .pipelineCreationCacheControl = VK_FALSE,
            .privateData = VK_FALSE,
            .shaderDemoteToHelperInvocation = VK_FALSE,
            .shaderTerminateInvocation = VK_FALSE,
            .subgroupSizeControl = VK_FALSE,
            .computeFullSubgroups = VK_FALSE,
            .synchronization2 = VK_TRUE,
            .textureCompressionASTC_HDR = VK_FALSE,
            .shaderZeroInitializeWorkgroupMemory = VK_FALSE,
            .dynamicRendering = VK_TRUE,
            .shaderIntegerDotProduct = VK_FALSE,
            .maintenance4 = VK_FALSE
        };
        enabled12.pNext = &enabled13;

        auto enabledExtensions = std::vector<const char*>{};
        enabledExtensions.reserve(enabledExtensionNames.size());
        for (const auto& enabledExtensionName : enabledExtensionNames)
            enabledExtensions.push_back(enabledExtensionName.c_str());

        VkPhysicalDeviceFeatures feats{
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
            .samplerAnisotropy = physicalDeviceFeatures.core.features.samplerAnisotropy,
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
            .pNext = &faultFeatures,
            .flags = 0,
            .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
            .pQueueCreateInfos = queueCreateInfos.data(),
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size()),
            .ppEnabledExtensionNames = enabledExtensions.data(),
            .pEnabledFeatures = &feats
        };

        if (vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device) != VK_SUCCESS)
            return std::unexpected(Error::InitializationFailed);

        queueFamilies.clear();
        queueFamilies.resize(queueFamilyProperties.size());

        for (const auto& queueCreateInfo : queueCreateInfos) {
            const uint32_t familyIndex = queueCreateInfo.queueFamilyIndex;
            queueFamilies[familyIndex] = std::vector<Queue>(queueCreateInfo.queueCount);

            for (uint32_t queueIndex = 0; queueIndex < queueCreateInfo.queueCount; ++queueIndex)
                vkGetDeviceQueue(device, familyIndex, queueIndex, &queueFamilies[familyIndex][queueIndex].queue);
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
            .vkGetMemoryWin32HandleKHR = vkGetMemoryWin32HandleKHR,
            .vkGetPhysicalDeviceProperties2KHR = vkGetPhysicalDeviceProperties2KHR
        };

        const VmaAllocatorCreateInfo allocatorInfo{
            .flags = 0,
            .physicalDevice = physicalDevice,
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
            return std::unexpected(Error::InitializationFailed);

        return {};
    }

    void VulkanRenderingDeviceDriver::releaseSwapchain(
        VulkanSwapchain* swapchain
    ) {
        // TODO: Use VK_EXT_swapchain_maintenance1 and VkSwapchainPresentFenceInfoKHR
        vkDeviceWaitIdle(device);

        if (!swapchain->blitFences.empty()) {
            vkWaitForFences(device, swapchain->blitFences.size(), swapchain->blitFences.data(), VK_TRUE,
                            std::numeric_limits<uint64_t>::max());

            for (const auto& fence : swapchain->blitFences)
                vkDestroyFence(device, fence, nullptr);
        }
        swapchain->blitFences.clear();

        for (const auto& semaphore : swapchain->blitSemaphores)
            vkDestroySemaphore(device, semaphore, nullptr);
        swapchain->blitSemaphores.clear();

        if (swapchain->blitCommandPool != nullptr) {
            vkFreeCommandBuffers(device, swapchain->blitCommandPool, swapchain->blitCommandBuffers.size(),
                                 swapchain->blitCommandBuffers.data());
            vkResetCommandPool(device, swapchain->blitCommandPool, 0);
            vkDestroyCommandPool(device, swapchain->blitCommandPool, nullptr);
        }
        swapchain->blitCommandBuffers.clear();
        swapchain->blitCommandPool = nullptr;

        for (uint32_t i = 0; i < swapchain->resolveImages.size(); i++) {
            delete swapchain->framebuffers[i];

            destroyImage(swapchain->colorTargets[i]);
            destroyImage(swapchain->depthTargets[i]);
            vkDestroyImageView(device, swapchain->resolveImageViews[i], nullptr);
        }

        swapchain->colorTargets.clear();
        swapchain->depthTargets.clear();
        swapchain->imageIndex = std::numeric_limits<uint32_t>::max();
        swapchain->resolveImages.clear();
        swapchain->resolveImageViews.clear();
        swapchain->framebuffers.clear();

        if (swapchain->swapchain != nullptr) {
            vkDestroySwapchainKHR(device, swapchain->swapchain, nullptr);
            swapchain->swapchain = nullptr;
        }

        for (uint32_t i = 0; i < swapchain->acquiredCommandQueues.size(); i++)
            recreateImageSemaphore(
                swapchain->acquiredCommandQueues[i],
                swapchain->acquiredCommandQueueSemaphores[i],
                false
            );

        swapchain->acquiredCommandQueues.clear();
        swapchain->acquiredCommandQueueSemaphores.clear();
    }

    auto VulkanRenderingDeviceDriver::releaseImageSemaphore(
        VulkanCommandQueue* commandQueue,
        const uint32_t semaphoreIndex,
        const bool releaseOnSwapchain
    ) -> std::expected<void, Error> {
        if (const auto swapchain = dynamic_cast<VulkanSwapchain*>(
                commandQueue->imageSemaphoresSwapchains[semaphoreIndex]);
            swapchain != nullptr) {
            commandQueue->imageSemaphoresSwapchains[semaphoreIndex] = nullptr;

            if (releaseOnSwapchain) {
                for (uint32_t i = 0; i < swapchain->acquiredCommandQueues.size(); i++) {
                    if (swapchain->acquiredCommandQueues[i] == commandQueue && swapchain->
                        acquiredCommandQueueSemaphores[i] == semaphoreIndex) {
                        swapchain->acquiredCommandQueues.erase(swapchain->acquiredCommandQueues.begin() + i);
                        swapchain->acquiredCommandQueueSemaphores.erase(
                            swapchain->acquiredCommandQueueSemaphores.begin() + i
                        );
                    }
                }
            }

            return {};
        }

        return std::unexpected(Error::InitializationFailed);
    }

    auto VulkanRenderingDeviceDriver::recreateImageSemaphore(
        VulkanCommandQueue* commandQueue,
        const uint32_t semaphoreIndex,
        const bool releaseOnSwapchain
    ) const -> std::expected<void, Error> {
        if (!releaseImageSemaphore(commandQueue, semaphoreIndex, releaseOnSwapchain))
            return std::unexpected(Error::InitializationFailed);

        constexpr VkSemaphoreCreateInfo semaphoreInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0
        };

        VkSemaphore semaphore;
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS)
            return std::unexpected(Error::InitializationFailed);

        vkDestroySemaphore(device, commandQueue->imageSemaphores[semaphoreIndex], nullptr);

        commandQueue->imageSemaphores[semaphoreIndex] = semaphore;
        commandQueue->freeImageSemaphores.push_back(semaphoreIndex);

        return {};
    }

    auto VulkanRenderingDeviceDriver::validateImageFormatSupport(
        const VkImageCreateInfo& info
    ) const -> std::expected<void, ResourceCreationError> {
        const VkPhysicalDeviceImageFormatInfo2 format{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
            .pNext = nullptr,
            .format = info.format,
            .type = info.imageType,
            .tiling = info.tiling,
            .usage = info.usage,
            .flags = info.flags
        };

        VkImageFormatProperties2 properties{
            .sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2,
            .pNext = nullptr,
            .imageFormatProperties = {}
        };
        const auto result = vkGetPhysicalDeviceImageFormatProperties2(physicalDevice, &format, &properties);

        if (result == VK_ERROR_FORMAT_NOT_SUPPORTED)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::UnsupportedFormat,
                    .message = "The requested image format/type/tiling/usage combination is not supported"
                }
            };

        if (result != VK_SUCCESS)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::CompatibilityError,
                    .message = "Failed to query physical-device image capabilities"
                }
            };

        const auto& limits = properties.imageFormatProperties;

        if (info.extent.width > limits.maxExtent.width ||
            info.extent.height > limits.maxExtent.height ||
            info.extent.depth > limits.maxExtent.depth) {
            const auto [limitName, requested, supported] = [&]()
                -> std::tuple<std::string_view, uint64_t, uint64_t> {
                    if (info.extent.width > limits.maxExtent.width)
                        return {"maxExtent.width", info.extent.width, limits.maxExtent.width};

                    if (info.extent.height > limits.maxExtent.height)
                        return {"maxExtent.height", info.extent.height, limits.maxExtent.height};

                    return {"maxExtent.depth", info.extent.depth, limits.maxExtent.depth};
                }();

            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::ExtentExceedsDeviceLimits,
                    .message = std::format(
                        "Request extent {}x{}x{} exceeds the supported maximum {}x{}x{}",
                        info.extent.width,
                        info.extent.height,
                        info.extent.depth,
                        limits.maxExtent.width,
                        limits.maxExtent.height,
                        limits.maxExtent.depth
                    ),
                    .limitViolation = ResourceCreationLimitViolation{
                        .limit = std::string{limitName},
                        .requested = requested,
                        .supported = supported
                    }
                }
            };
        }

        if (info.mipLevels > limits.maxMipLevels)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::MipCountExceedsDeviceLimits,
                    .message = std::format(
                        "Requested {} mip levels, but this image configuration supports at most {}",
                        info.mipLevels,
                        limits.maxMipLevels
                    ),
                    .limitViolation = ResourceCreationLimitViolation{
                        .limit = "maxMipLevels",
                        .requested = info.mipLevels,
                        .supported = limits.maxMipLevels
                    }
                }
            };

        if (info.arrayLayers > limits.maxArrayLayers)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::LayerCountExceedsDeviceLimits,
                    .message = std::format(
                        "Requested {} array layers, but this image configuration supports at most {}",
                        info.arrayLayers,
                        limits.maxArrayLayers
                    ),
                    .limitViolation = ResourceCreationLimitViolation{
                        .limit = "maxArrayLayers",
                        .requested = info.arrayLayers,
                        .supported = limits.maxArrayLayers
                    }
                }
            };

        if ((limits.sampleCounts & info.samples) == 0)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::UnsupportedSampleCount,
                    .message = "The requested sample count is not supported for this image configuration"
                }
            };

        return {};
    }

    VulkanRenderingDeviceDriver::VulkanRenderingDeviceDriver(
        VulkanRenderingContextDriver* renderingContext,
        const uint32_t deviceIndex,
        const uint32_t frameCount
    ) : RenderingDeviceDriver(),
        enabledFeatures(),
        renderingContext(renderingContext),
        deviceIndex(deviceIndex),
        physicalDevice(renderingContext->getPhysicalDevice(deviceIndex)),
        physicalDeviceFeatures({}),
        physicalDeviceProperties({}),
        maxBufferSize(0),
        device(VK_NULL_HANDLE),
        allocator(VK_NULL_HANDLE),
        frameCount(frameCount) {
        VkPhysicalDeviceMaintenance4Properties maintenance4Properties{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_PROPERTIES,
            .pNext = nullptr,
            .maxBufferSize = 0
        };
        VkPhysicalDeviceProperties2 properties{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            .pNext = &maintenance4Properties,
            .properties = {}
        };
        vkGetPhysicalDeviceProperties2(physicalDevice, &properties);
        physicalDeviceProperties = properties.properties;
        maxBufferSize = maintenance4Properties.maxBufferSize;

        physicalDeviceFeatures.core.pNext = &physicalDeviceFeatures.vulkan12;
        physicalDeviceFeatures.vulkan12.pNext = &physicalDeviceFeatures.vulkan13;

        vkGetPhysicalDeviceFeatures2(physicalDevice, &physicalDeviceFeatures.core);

        const auto queueFamilyCount = renderingContext->getQueueFamilyCount(deviceIndex);
        queueFamilyProperties.resize(queueFamilyCount);
        for (uint32_t i = 0; i < queueFamilyCount; i++)
            queueFamilyProperties[i] = renderingContext->getQueueFamilyProperties(deviceIndex, i);

        if (!initializeExtensions())
            error<CantCreateError>("A required extension is not supported by the requested device.");

        checkFeatures();

        checkCapabilities();

        if (!initializeDevice())
            error<CantCreateError>("Failed to create virtual device.");
    }

    VulkanRenderingDeviceDriver::~VulkanRenderingDeviceDriver() {
        vmaDestroyAllocator(allocator);

        vkDestroyDevice(device, nullptr);
    }

    auto VulkanRenderingDeviceDriver::createSwapchain(
        Surface* surface
    ) -> std::expected<Swapchain*, Error> {
        DEBUG_ASSERT(surface != nullptr);

        const auto vkSurface = dynamic_cast<VulkanSurface*>(surface);

        uint32_t formatCount;
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, vkSurface->surface, &formatCount, nullptr)
            != VK_SUCCESS)
            return std::unexpected(Error::InitializationFailed);

        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice,
            vkSurface->surface,
            &formatCount,
            formats.data()
        ) != VK_SUCCESS)
            return std::unexpected(Error::InitializationFailed);

        VkFormat format = VK_FORMAT_UNDEFINED;
        VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

        if (formatCount == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
            format = VK_FORMAT_B8G8R8A8_SRGB;
            colorSpace = formats[0].colorSpace;
        } else if (formatCount > 0) {
            constexpr VkFormat preferredFormat = VK_FORMAT_B8G8R8A8_UNORM;
            constexpr VkFormat alternativeFormat = VK_FORMAT_R8G8B8A8_UNORM;

            for (uint32_t i = 0; i < formatCount; i++) {
                if (formats[i].format == preferredFormat || formats[i].format == alternativeFormat) {
                    format = formats[i].format;
                    colorSpace = formats[i].colorSpace;

                    if (formats[i].format == preferredFormat)
                        break;
                }
            }
        }

        if (format == VK_FORMAT_UNDEFINED)
            return std::unexpected(Error::InitializationFailed);

        auto* swapchain = new VulkanSwapchain();
        swapchain->surface = vkSurface;
        swapchain->format = format;
        swapchain->colorSpace = colorSpace;

        return swapchain;
    }

    auto VulkanRenderingDeviceDriver::resizeSwapchain(
        CommandQueue* commandQueue,
        Swapchain* swapchain,
        const uint32_t imageCount
    ) -> std::expected<void, Error> {
        DEBUG_ASSERT(commandQueue != nullptr);
        DEBUG_ASSERT(swapchain != nullptr);

        const auto vkSwapchain = dynamic_cast<VulkanSwapchain*>(swapchain);
        const auto vkGraphicsQueue = dynamic_cast<VulkanCommandQueue*>(commandQueue);
        if (vkSwapchain == nullptr || vkGraphicsQueue == nullptr)
            return std::unexpected(Error::InitializationFailed);

        if ((queueFamilyProperties[vkGraphicsQueue->queueFamily].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
            return std::unexpected(Error::InitializationFailed);

        const auto presentQueueFamily = getQueueFamily({}, vkSwapchain->surface);
        if (!presentQueueFamily)
            return std::unexpected(presentQueueFamily.error());

        vkSwapchain->graphicsQueueFamily = vkGraphicsQueue->queueFamily;
        vkSwapchain->presentQueueFamily = presentQueueFamily.value();
        releaseSwapchain(vkSwapchain);

        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            physicalDevice,
            vkSwapchain->surface->surface,
            &surfaceCapabilities
        ) != VK_SUCCESS)
            return std::unexpected(Error::InitializationFailed);

        const auto surface = vkSwapchain->surface;
        if (!renderingContext->deviceSupportsPresent(deviceIndex, surface))
            return std::unexpected(Error::InitializationFailed);

        if (!vkSwapchain->swapchain) {
            if (surfaceCapabilities.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
                surfaceCapabilities.currentExtent.width = std::clamp(
                    surface->resolution.x,
                    surfaceCapabilities.minImageExtent.width,
                    surfaceCapabilities.maxImageExtent.width
                );
                surfaceCapabilities.currentExtent.height = std::clamp(
                    surface->resolution.y,
                    surfaceCapabilities.minImageExtent.height,
                    surfaceCapabilities.maxImageExtent.height
                );
            }

            if (surfaceCapabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR ||
                surfaceCapabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) {
                std::swap(surfaceCapabilities.currentExtent.width, surfaceCapabilities.currentExtent.height);
            }
        }

        VkExtent2D extent;
        if (surfaceCapabilities.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
            extent.width = std::clamp(
                surface->resolution.x,
                surfaceCapabilities.minImageExtent.width,
                surfaceCapabilities.maxImageExtent.width
            );
            extent.height = std::clamp(
                surface->resolution.y,
                surfaceCapabilities.minImageExtent.height,
                surfaceCapabilities.maxImageExtent.height
            );
        } else {
            extent = surfaceCapabilities.currentExtent;
            surface->resolution.x = extent.width;
            surface->resolution.y = extent.height;
        }

        if (surface->resolution.x == 0 || surface->resolution.y == 0)
            return {};

        const std::array queueFamilyIndices{
            vkSwapchain->graphicsQueueFamily,
            vkSwapchain->presentQueueFamily
        };
        const bool hasSeparatePresentQueue = queueFamilyIndices[0] != queueFamilyIndices[1];

        VkSwapchainCreateInfoKHR swapchainInfo{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .surface = surface->surface,
            .minImageCount = std::max(surfaceCapabilities.minImageCount, imageCount),
            .imageFormat = vkSwapchain->format,
            .imageColorSpace = vkSwapchain->colorSpace,
            .imageExtent = {
                .width = surface->resolution.x,
                .height = surface->resolution.y
            },
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .imageSharingMode = hasSeparatePresentQueue ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = hasSeparatePresentQueue ? static_cast<uint32_t>(queueFamilyIndices.size()) : 0,
            .pQueueFamilyIndices = hasSeparatePresentQueue ? queueFamilyIndices.data() : nullptr,
            .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            // TODO: Add support for transparent frames, useful for e.g. splash screens with transparent backgrounds.
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = VK_PRESENT_MODE_FIFO_KHR,
            .clipped = VK_TRUE,
            .oldSwapchain = nullptr
        };

        std::vector<VkPresentModeKHR> supportedPresentModes{};
        uint32_t presentModeCount;
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice,
            surface->surface,
            &presentModeCount,
            nullptr
        ) != VK_SUCCESS)
            return std::unexpected(Error::InitializationFailed);
        supportedPresentModes.resize(presentModeCount);
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice,
            surface->surface,
            &presentModeCount,
            supportedPresentModes.data()
        ) != VK_SUCCESS)
            return std::unexpected(Error::InitializationFailed);

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
            return std::unexpected(Error::InitializationFailed);

        uint32_t swapchainImageCount;
        if (vkGetSwapchainImagesKHR(device, vkSwapchain->swapchain, &swapchainImageCount, nullptr) != VK_SUCCESS)
            return std::unexpected(Error::InitializationFailed);
        vkSwapchain->resolveImages.resize(swapchainImageCount);
        vkSwapchain->resolveImageViews.resize(swapchainImageCount);
        if (vkGetSwapchainImagesKHR(
            device,
            vkSwapchain->swapchain,
            &swapchainImageCount,
            vkSwapchain->resolveImages.
                         data()
        ) != VK_SUCCESS)
            return std::unexpected(Error::InitializationFailed);

        vkSwapchain->colorTargets.resize(swapchainImageCount);
        vkSwapchain->depthTargets.resize(swapchainImageCount);
        vkSwapchain->framebuffers.resize(swapchainImageCount);
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
                return std::unexpected(Error::InitializationFailed);

            auto colorTarget = createImage(
                {
                    .format = static_cast<ImageDataFormat>(vkSwapchain->format - 1),
                    .width = swapchainInfo.imageExtent.width,
                    .height = swapchainInfo.imageExtent.height,
                    .depth = 1,
                    .layerCount = 1,
                    .mipmapCount = 1,
                    .type = ImageType::TwoD,
                    .samples = ImageSamples::One,
                    .usage = ImageUsageBits::ColorAttachment | ImageUsageBits::CopySource
                },
                {
                    .format = static_cast<ImageDataFormat>(vkSwapchain->format - 1),
                    .swizzleRed = ImageSwizzle::Red,
                    .swizzleGreen = ImageSwizzle::Green,
                    .swizzleBlue = ImageSwizzle::Blue,
                    .swizzleAlpha = ImageSwizzle::Alpha
                }
            );
            auto depthTarget = createImage(
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
                    .usage = ImageUsageBits::DepthStencilAttachment
                },
                {
                    .format = D32_SFLOAT_S8_UINT,
                    .swizzleRed = ImageSwizzle::Red,
                    .swizzleGreen = ImageSwizzle::Green,
                    .swizzleBlue = ImageSwizzle::Blue,
                    .swizzleAlpha = ImageSwizzle::Alpha
                }
            );

            vkSwapchain->colorTargets[i] = dynamic_cast<VulkanImage*>(colorTarget.value());
            vkSwapchain->depthTargets[i] = dynamic_cast<VulkanImage*>(depthTarget.value());

            const auto framebuffer = new VulkanFramebuffer();
            framebuffer->colorTarget = vkSwapchain->colorTargets[i];
            framebuffer->depthTarget = vkSwapchain->depthTargets[i];
            framebuffer->resolveImage = vkSwapchain->resolveImages[i];
            framebuffer->resolveImageView = vkSwapchain->resolveImageViews[i];
            framebuffer->resolveSubresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            };
            framebuffer->swapchainAcquired = false;
            vkSwapchain->framebuffers[i] = framebuffer;

            VkFence fence;
            constexpr VkFenceCreateInfo fenceInfo{
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT
            };
            if (vkCreateFence(device, &fenceInfo, nullptr, &fence) != VK_SUCCESS)
                return std::unexpected(Error::InitializationFailed);
            vkSwapchain->blitFences.push_back(fence);

            VkSemaphore semaphore = VK_NULL_HANDLE;
            VkSemaphoreCreateInfo semaphoreInfo{
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0
            };
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS)
                return std::unexpected(Error::InitializationFailed);
            vkSwapchain->blitSemaphores.push_back(semaphore);
        }

        renderingContext->setSurfaceNeedsResize(surface, false);

        return {};
    }

    auto VulkanRenderingDeviceDriver::acquireSwapchainFramebuffer(
        CommandQueue* commandQueue,
        Swapchain* swapchain
    ) -> std::expected<Framebuffer*, SwapchainError> {
        DEBUG_ASSERT(commandQueue != nullptr);
        DEBUG_ASSERT(swapchain != nullptr);

        const auto vkCommandQueue = dynamic_cast<VulkanCommandQueue*>(commandQueue);
        const auto vkSwapchain = dynamic_cast<VulkanSwapchain*>(swapchain);

        if (vkSwapchain->swapchain == VK_NULL_HANDLE || renderingContext->getSurfaceNeedsResize(vkSwapchain->surface))
            return std::unexpected(SwapchainError::ResizeRequired);

        VkSemaphore semaphore = VK_NULL_HANDLE;
        uint32_t semaphoreIndex;
        if (vkCommandQueue->freeImageSemaphores.empty()) {
            constexpr VkSemaphoreCreateInfo semaphoreInfo{
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0
            };

            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS)
                return std::unexpected(SwapchainError::Failed);

            semaphoreIndex = vkCommandQueue->imageSemaphores.size();
            vkCommandQueue->imageSemaphores.push_back(semaphore);
            vkCommandQueue->imageSemaphoresSwapchains.push_back(swapchain);
        } else {
            const uint32_t freeIndex = vkCommandQueue->freeImageSemaphores.size() - 1;
            semaphoreIndex = vkCommandQueue->freeImageSemaphores[freeIndex];
            vkCommandQueue->imageSemaphoresSwapchains[semaphoreIndex] = swapchain;
            vkCommandQueue->freeImageSemaphores.erase(vkCommandQueue->freeImageSemaphores.begin() + freeIndex);
            semaphore = vkCommandQueue->imageSemaphores[semaphoreIndex];
        }

        vkSwapchain->acquiredCommandQueues.push_back(vkCommandQueue);
        vkSwapchain->acquiredCommandQueueSemaphores.push_back(semaphoreIndex);

        const auto result = vkAcquireNextImageKHR(
            device,
            vkSwapchain->swapchain,
            std::numeric_limits<uint64_t>::max(),
            semaphore,
            VK_NULL_HANDLE,
            &vkSwapchain->imageIndex
        );
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            if (!recreateImageSemaphore(vkCommandQueue, semaphoreIndex, true))
                return std::unexpected(SwapchainError::Failed);

            return std::unexpected(SwapchainError::ResizeRequired);
        }
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            return std::unexpected(SwapchainError::Failed);

        vkCommandQueue->pendingSemaphoresForExecute.push_back(semaphoreIndex);
        vkCommandQueue->pendingSemaphoresForFence.push_back(semaphoreIndex);

        const auto framebuffer = vkSwapchain->framebuffers[vkSwapchain->imageIndex];
        framebuffer->swapchainAcquired = true;
        return framebuffer;
    }

    void VulkanRenderingDeviceDriver::destroySwapchain(
        Swapchain* swapchain
    ) {
        DEBUG_ASSERT(swapchain != nullptr);

        const auto vkSwapchain = dynamic_cast<VulkanSwapchain*>(swapchain);
        releaseSwapchain(vkSwapchain);
        delete vkSwapchain;
    }

    auto VulkanRenderingDeviceDriver::getQueueFamily(
        const QueueFamilyFlags queueFamilyFlags,
        Surface* surface
    ) -> std::expected<uint32_t, Error> {
        const VkQueueFlags requiredFlags = toVkQueueFlags(queueFamilyFlags);

        uint32_t pickedExtraFlagCount = std::numeric_limits<uint32_t>::max();
        uint32_t pickedQueueFamilyIndex = std::numeric_limits<uint32_t>::max();

        for (uint32_t i = 0; i < queueFamilyProperties.size(); i++) {
            if (i >= queueFamilies.size() || queueFamilies[i].empty())
                continue;

            if (surface != nullptr &&
                !VulkanRenderingContextDriver::queueFamilySupportsPresent(
                    physicalDevice,
                    i,
                    dynamic_cast<VulkanSurface*>(surface)
                ))
                continue;

            const VkQueueFlags availableFlags = queueFamilyProperties[i].queueFlags;
            if ((availableFlags & requiredFlags) != requiredFlags)
                continue;

            const VkQueueFlags extraFlags = availableFlags & ~requiredFlags;

            const uint32_t extraFlagCount = std::popcount(extraFlags);

            if (extraFlagCount < pickedExtraFlagCount) {
                pickedQueueFamilyIndex = i;
                pickedExtraFlagCount = extraFlagCount;
            }
        }

        if (pickedQueueFamilyIndex == std::numeric_limits<uint32_t>::max())
            return std::unexpected(Error::InitializationFailed);

        return pickedQueueFamilyIndex;
    }

    auto VulkanRenderingDeviceDriver::createFence() -> std::expected<Fence*, Error> {
        constexpr VkFenceCreateInfo fenceInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0
        };

        VkFence o;
        if (vkCreateFence(device, &fenceInfo, nullptr, &o) != VK_SUCCESS)
            return std::unexpected(Error::InitializationFailed);

        const auto fence = new VulkanFence();
        fence->fence = o;
        return fence;
    }

    auto VulkanRenderingDeviceDriver::waitOnFence(
        Fence* fence
    ) -> std::expected<void, Error> {
        const auto vkFence = dynamic_cast<VulkanFence*>(fence);

        if (vkWaitForFences(device, 1, &vkFence->fence, VK_TRUE, std::numeric_limits<uint64_t>::max()) != VK_SUCCESS)
            return std::unexpected(Error::InitializationFailed);

        if (vkResetFences(device, 1, &vkFence->fence) != VK_SUCCESS)
            return std::unexpected(Error::InitializationFailed);

        if (vkFence->queueSignaledFrom) {
            auto& pairs = vkFence->queueSignaledFrom->imageSemaphoresForFences;
            uint32_t i = 0;
            while (i < pairs.size()) {
                if (pairs[i].first == vkFence) {
                    if (!releaseImageSemaphore(vkFence->queueSignaledFrom, pairs[i].second, true))
                        return std::unexpected(Error::InitializationFailed);

                    vkFence->queueSignaledFrom->freeImageSemaphores.push_back(pairs[i].second);
                    pairs.erase(pairs.begin() + i);
                } else {
                    i++;
                }
            }

            vkFence->queueSignaledFrom = nullptr;
        }

        return {};
    }

    void VulkanRenderingDeviceDriver::destroyFence(
        Fence* fence
    ) {
        const auto o = dynamic_cast<VulkanFence*>(fence);
        vkDestroyFence(device, o->fence, nullptr);
        delete o;
    }

    auto VulkanRenderingDeviceDriver::createSemaphore() -> std::expected<Semaphore*, Error> {
        VkSemaphoreTypeCreateInfo semaphoreTypeInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
            .pNext = nullptr,
            .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
            .initialValue = 0
        };
        const VkSemaphoreCreateInfo semaphoreInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = &semaphoreTypeInfo,
            .flags = 0
        };

        VkSemaphore o;
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &o) != VK_SUCCESS)
            return std::unexpected(Error::InitializationFailed);

        const auto semaphore = new VulkanSemaphore();
        semaphore->semaphore = o;
        return semaphore;
    }

    void VulkanRenderingDeviceDriver::destroySemaphore(
        Semaphore* semaphore
    ) {
        const auto o = dynamic_cast<VulkanSemaphore*>(semaphore);
        vkDestroySemaphore(device, o->semaphore, nullptr);
        delete o;
    }

    auto VulkanRenderingDeviceDriver::createCommandPool(
        const uint32_t queueFamily,
        const CommandBufferType type
    ) -> std::expected<CommandPool*, Error> {
        const VkCommandPoolCreateInfo commandPoolInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = queueFamily
        };

        VkCommandPool o;
        if (vkCreateCommandPool(device, &commandPoolInfo, nullptr, &o) != VK_SUCCESS)
            return std::unexpected(Error::InitializationFailed);

        const auto commandPool = new VulkanCommandPool{};
        commandPool->pool = o;
        commandPool->type = type;
        commandPool->queueFamily = queueFamily;
        return commandPool;
    }

    auto VulkanRenderingDeviceDriver::resetCommandPool(
        CommandPool* pool
    ) -> std::expected<void, Error> {
        if (vkResetCommandPool(device, dynamic_cast<VulkanCommandPool*>(pool)->pool, 0) != VK_SUCCESS)
            return std::unexpected(Error::InitializationFailed);

        return {};
    }

    void VulkanRenderingDeviceDriver::destroyCommandPool(
        CommandPool* pool
    ) {
        const auto* o = dynamic_cast<VulkanCommandPool*>(pool);
        vkDestroyCommandPool(device, o->pool, nullptr);
        delete o;
    }

    auto VulkanRenderingDeviceDriver::createCommandBuffer(
        CommandPool* pool
    ) -> std::expected<CommandBuffer*, Error> {
        const auto* p = dynamic_cast<VulkanCommandPool*>(pool);

        const VkCommandBufferAllocateInfo commandBufferInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = p->pool,
            .level = requireVkConversion(toVkCommandBufferLevel(p->type)),
            .commandBufferCount = 1
        };

        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        if (vkAllocateCommandBuffers(device, &commandBufferInfo, &commandBuffer) != VK_SUCCESS)
            return std::unexpected(Error::InitializationFailed);

        const auto o = new VulkanCommandBuffer();
        o->commandBuffer = commandBuffer;
        o->renderingAttachmentScratch.reserve(physicalDeviceProperties.limits.maxColorAttachments);

        return o;
    }

    auto VulkanRenderingDeviceDriver::beginCommandBuffer(
        CommandBuffer* commandBuffer
    ) -> std::expected<void, Error> {
        const auto o = dynamic_cast<VulkanCommandBuffer*>(commandBuffer);

        constexpr VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr
        };
        if (vkBeginCommandBuffer(o->commandBuffer, &beginInfo) != VK_SUCCESS)
            return std::unexpected(Error::InitializationFailed);

        return {};
    }

    void VulkanRenderingDeviceDriver::endCommandBuffer(
        CommandBuffer* commandBuffer
    ) {
        const auto o = dynamic_cast<VulkanCommandBuffer*>(commandBuffer);
        vkEndCommandBuffer(o->commandBuffer);
    }

    auto VulkanRenderingDeviceDriver::createCommandQueue(
        const uint32_t queueFamilyIndex
    ) -> std::expected<CommandQueue*, Error> {
        std::vector<Queue>& queueFamily = queueFamilies[queueFamilyIndex];
        uint32_t pickedQueueFamilyIndex = std::numeric_limits<uint32_t>::max();
        uint32_t pickedVirtualCount = std::numeric_limits<uint32_t>::max();
        for (uint32_t i = 0; i < queueFamily.size(); i++) {
            if (queueFamily[i].virtualCount < pickedVirtualCount) {
                pickedQueueFamilyIndex = i;
                pickedVirtualCount = queueFamily[i].virtualCount;
            }
        }

        if (pickedQueueFamilyIndex >= queueFamily.size())
            return std::unexpected(Error::InitializationFailed);

        auto commandQueue = new VulkanCommandQueue();
        commandQueue->queueFamily = queueFamilyIndex;
        commandQueue->queueIndex = pickedQueueFamilyIndex;
        queueFamily[pickedQueueFamilyIndex].virtualCount++;

        return commandQueue;
    }

    auto VulkanRenderingDeviceDriver::executeCommandQueueAndPresent(
        CommandQueue* commandQueue,
        const std::vector<Semaphore*>& waitSemaphores,
        const std::vector<CommandBuffer*>& commandBuffers,
        const std::vector<Semaphore*>& signalSemaphores,
        Fence* fence,
        const std::vector<Swapchain*>& swapchains
    ) -> std::expected<void, Error> {
        const auto vkCommandQueue = dynamic_cast<VulkanCommandQueue*>(commandQueue);
        if (vkCommandQueue == nullptr)
            return std::unexpected(Error::InitializationFailed);

        if (!swapchains.empty() &&
            (queueFamilyProperties[vkCommandQueue->queueFamily].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
            return std::unexpected(Error::InitializationFailed);

        for (const auto swapchain : swapchains) {
            const auto vkSwapchain = dynamic_cast<VulkanSwapchain*>(swapchain);
            if (vkSwapchain == nullptr || vkSwapchain->graphicsQueueFamily != vkCommandQueue->queueFamily ||
                vkSwapchain->presentQueueFamily >= queueFamilies.size() ||
                queueFamilies[vkSwapchain->presentQueueFamily].empty() ||
                !VulkanRenderingContextDriver::queueFamilySupportsPresent(
                    physicalDevice,
                    vkSwapchain->presentQueueFamily,
                    vkSwapchain->surface
                ))
                return std::unexpected(Error::InitializationFailed);
        }

        Queue& queue = queueFamilies[vkCommandQueue->queueFamily][vkCommandQueue->queueIndex];
        const auto vkFence = dynamic_cast<VulkanFence*>(fence);

        const auto associatePendingImageSemaphoresWithFence = [&] {
            if (vkFence == nullptr || vkCommandQueue->pendingSemaphoresForFence.empty())
                return;

            vkFence->queueSignaledFrom = vkCommandQueue;
            for (const uint32_t semaphoreIndex : vkCommandQueue->pendingSemaphoresForFence)
                vkCommandQueue->imageSemaphoresForFences.emplace_back(vkFence, semaphoreIndex);

            vkCommandQueue->pendingSemaphoresForFence.clear();
        };

        std::vector<VkSemaphoreSubmitInfo> waitSemaphoreInfos{};
        waitSemaphoreInfos.reserve(waitSemaphores.size());

        std::vector<VkSemaphoreSubmitInfo> acquireSemaphoreInfos{};
        acquireSemaphoreInfos.reserve(vkCommandQueue->pendingSemaphoresForExecute.size());

        if (!swapchains.empty() && !vkCommandQueue->pendingSemaphoresForExecute.empty()) {
            for (const auto semaphoreIndex : vkCommandQueue->pendingSemaphoresForExecute) {
                acquireSemaphoreInfos.push_back({
                    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                    .pNext = nullptr,
                    .semaphore = vkCommandQueue->imageSemaphores[semaphoreIndex],
                    .value = 0,
                    .stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                    .deviceIndex = 0
                });
            }

            vkCommandQueue->pendingSemaphoresForExecute.clear();
        }

        for (const auto& semaphore : waitSemaphores) {
            const auto vkSemaphore = dynamic_cast<VulkanSemaphore*>(semaphore);
            auto& destination = commandBuffers.empty() && !swapchains.empty()
                                    ? acquireSemaphoreInfos
                                    : waitSemaphoreInfos;
            destination.push_back({
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .pNext = nullptr,
                .semaphore = vkSemaphore->semaphore,
                .value = vkSemaphore->value,
                .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                .deviceIndex = 0
            });
        }

        if (!commandBuffers.empty()) {
            std::vector<VkCommandBufferSubmitInfo> commandBufferInfos{};
            commandBufferInfos.reserve(commandBuffers.size());

            std::vector<VkSemaphoreSubmitInfo> signalSemaphoreInfos{};
            signalSemaphoreInfos.reserve(signalSemaphores.size());

            for (const auto& commandBuffer : commandBuffers) {
                commandBufferInfos.push_back({
                    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                    .pNext = nullptr,
                    .commandBuffer = dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->commandBuffer,
                    .deviceMask = 0
                });
            }

            for (const auto& semaphore : signalSemaphores) {
                const auto vkSemaphore = dynamic_cast<VulkanSemaphore*>(semaphore);
                signalSemaphoreInfos.push_back({
                    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                    .pNext = nullptr,
                    .semaphore = vkSemaphore->semaphore,
                    .value = ++vkSemaphore->value,
                    .stageMask = 0,
                    .deviceIndex = 0
                });
            }

            const VkSubmitInfo2 submitInfo{
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                .pNext = nullptr,
                .flags = 0,
                .waitSemaphoreInfoCount = static_cast<uint32_t>(waitSemaphoreInfos.size()),
                .pWaitSemaphoreInfos = waitSemaphoreInfos.data(),
                .commandBufferInfoCount = static_cast<uint32_t>(commandBufferInfos.size()),
                .pCommandBufferInfos = commandBufferInfos.data(),
                .signalSemaphoreInfoCount = static_cast<uint32_t>(signalSemaphoreInfos.size()),
                .pSignalSemaphoreInfos = signalSemaphoreInfos.data()
            };

            queue.submitMutex.lock();
            const auto submitResult = vkQueueSubmit2(
                queue.queue,
                1,
                &submitInfo,
                vkFence != nullptr && swapchains.empty() ? vkFence->fence : VK_NULL_HANDLE
            );
            queue.submitMutex.unlock();

            if (submitResult == VK_ERROR_DEVICE_LOST) {
                // TODO: Print crash log
                CRASH("Vulkan device lost");
            }
            if (submitResult != VK_SUCCESS)
                return std::unexpected(Error::InitializationFailed);

            if (swapchains.empty())
                associatePendingImageSemaphoresWithFence();

            waitSemaphoreInfos.clear();
        }

        if (!swapchains.empty()) {
            std::vector<VkSemaphore> presentWaitSemaphores{};
            presentWaitSemaphores.reserve(swapchains.size());

            bool firstPresentSubmit = true;

            for (const auto& swapchain : swapchains) {
                const auto vkSwapchain = dynamic_cast<VulkanSwapchain*>(swapchain);

                if (vkSwapchain->blitCommandPool == VK_NULL_HANDLE) {
                    const VkCommandPoolCreateInfo poolInfo{
                        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                        .pNext = nullptr,
                        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                        .queueFamilyIndex = vkCommandQueue->queueFamily
                    };

                    if (vkCreateCommandPool(device, &poolInfo, nullptr, &vkSwapchain->blitCommandPool) != VK_SUCCESS)
                        return std::unexpected(Error::InitializationFailed);

                    const auto commandBufferCount = static_cast<uint32_t>(vkSwapchain->resolveImages.size());

                    const VkCommandBufferAllocateInfo commandBufferInfo{
                        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                        .pNext = nullptr,
                        .commandPool = vkSwapchain->blitCommandPool,
                        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                        .commandBufferCount = commandBufferCount
                    };

                    vkSwapchain->blitCommandBuffers.resize(commandBufferCount);

                    if (vkAllocateCommandBuffers(device, &commandBufferInfo, vkSwapchain->blitCommandBuffers.data())
                        != VK_SUCCESS)
                        return std::unexpected(Error::InitializationFailed);
                }

                const uint32_t imageIndex = vkSwapchain->imageIndex;

                if (imageIndex >= vkSwapchain->blitCommandBuffers.size() ||
                    imageIndex >= vkSwapchain->blitFences.size() ||
                    imageIndex >= vkSwapchain->blitSemaphores.size()) {
                    return std::unexpected(Error::InitializationFailed);
                }

                const auto blitFence = vkSwapchain->blitFences[imageIndex];
                const auto commandBuffer = vkSwapchain->blitCommandBuffers[imageIndex];

                if (vkWaitForFences(device, 1, &blitFence, VK_TRUE, std::numeric_limits<uint64_t>::max()) !=
                    VK_SUCCESS)
                    return std::unexpected(Error::InitializationFailed);

                if (vkResetFences(device, 1, &blitFence) != VK_SUCCESS)
                    return std::unexpected(Error::InitializationFailed);

                if (vkResetCommandBuffer(commandBuffer, 0) != VK_SUCCESS)
                    return std::unexpected(Error::InitializationFailed);

                const VkCommandBufferBeginInfo beginInfo{
                    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                    .pNext = nullptr,
                    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                    .pInheritanceInfo = nullptr
                };

                if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
                    return std::unexpected(Error::InitializationFailed);

                VkImageMemoryBarrier transferSrcBarrier{
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .pNext = nullptr,
                    .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = vkSwapchain->colorTargets[imageIndex]->image,
                    .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1
                    }
                };

                vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    1,
                    &transferSrcBarrier
                );

                VkImageMemoryBarrier presentBarrier{
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .pNext = nullptr,
                    .srcAccessMask = 0,
                    .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = vkSwapchain->resolveImages[imageIndex],
                    .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1
                    }
                };

                vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    1,
                    &presentBarrier
                );

                const VkImageCopy region{
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
                        .width = vkSwapchain->surface->resolution.x,
                        .height = vkSwapchain->surface->resolution.y,
                        .depth = 1
                    }
                };

                vkCmdCopyImage(
                    commandBuffer,
                    vkSwapchain->colorTargets[imageIndex]->image,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    vkSwapchain->resolveImages[imageIndex],
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1,
                    &region
                );

                presentBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                presentBarrier.dstAccessMask = 0;
                presentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

                vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                    0,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    1,
                    &presentBarrier
                );

                transferSrcBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                transferSrcBarrier.dstAccessMask = 0;
                transferSrcBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                transferSrcBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                    0,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    1,
                    &transferSrcBarrier
                );

                if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
                    return std::unexpected(Error::InitializationFailed);

                const VkCommandBufferSubmitInfo commandBufferInfo{
                    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                    .pNext = nullptr,
                    .commandBuffer = commandBuffer,
                    .deviceMask = 0
                };

                const auto presentSemaphore = vkSwapchain->blitSemaphores[imageIndex];

                const VkSemaphoreSubmitInfo presentSignalInfo{
                    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                    .pNext = nullptr,
                    .semaphore = presentSemaphore,
                    .value = 0,
                    .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                    .deviceIndex = 0
                };

                const uint32_t waitCount = firstPresentSubmit
                                               ? static_cast<uint32_t>(acquireSemaphoreInfos.size())
                                               : 0;

                const VkSemaphoreSubmitInfo* waits = waitCount > 0
                                                         ? acquireSemaphoreInfos.data()
                                                         : nullptr;

                const VkSubmitInfo2 submitInfo{
                    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                    .pNext = nullptr,
                    .flags = 0,
                    .waitSemaphoreInfoCount = waitCount,
                    .pWaitSemaphoreInfos = waits,
                    .commandBufferInfoCount = 1,
                    .pCommandBufferInfos = &commandBufferInfo,
                    .signalSemaphoreInfoCount = 1,
                    .pSignalSemaphoreInfos = &presentSignalInfo
                };

                queue.submitMutex.lock();

                const VkResult submitResult = vkQueueSubmit2(
                    queue.queue,
                    1,
                    &submitInfo,
                    blitFence
                );

                queue.submitMutex.unlock();

                if (submitResult == VK_ERROR_DEVICE_LOST) {
                    CRASH("Vulkan device lost");
                }

                if (submitResult != VK_SUCCESS)
                    return std::unexpected(Error::InitializationFailed);

                firstPresentSubmit = false;

                presentWaitSemaphores.push_back(presentSemaphore);
            }

            if (vkFence != nullptr) {
                constexpr VkSubmitInfo2 fenceSubmitInfo{
                    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                    .pNext = nullptr,
                    .flags = 0,
                    .waitSemaphoreInfoCount = 0,
                    .pWaitSemaphoreInfos = nullptr,
                    .commandBufferInfoCount = 0,
                    .pCommandBufferInfos = nullptr,
                    .signalSemaphoreInfoCount = 0,
                    .pSignalSemaphoreInfos = nullptr
                };

                queue.submitMutex.lock();
                const VkResult fenceSubmitResult = vkQueueSubmit2(queue.queue, 1, &fenceSubmitInfo, vkFence->fence);
                queue.submitMutex.unlock();

                if (fenceSubmitResult == VK_ERROR_DEVICE_LOST) {
                    CRASH("Vulkan device lost");
                }
                if (fenceSubmitResult != VK_SUCCESS)
                    return std::unexpected(Error::InitializationFailed);

                associatePendingImageSemaphoresWithFence();
            }

            bool resizeRequired = false;
            bool presentationFailed = false;

            std::map<uint32_t, std::vector<uint32_t>> swapchainsByPresentFamily{};
            for (uint32_t i = 0; i < swapchains.size(); ++i) {
                const auto vkSwapchain = dynamic_cast<VulkanSwapchain*>(swapchains[i]);
                swapchainsByPresentFamily[vkSwapchain->presentQueueFamily].push_back(i);
            }

            for (const auto& [presentFamily, swapchainIndices] : swapchainsByPresentFamily) {
                std::vector<VkSwapchainKHR> vkSwapchains{};
                std::vector<uint32_t> imageIndices{};
                std::vector<VkSemaphore> waitSemaphoresForFamily{};
                std::vector<VkResult> results(swapchainIndices.size());

                vkSwapchains.reserve(swapchainIndices.size());
                imageIndices.reserve(swapchainIndices.size());
                waitSemaphoresForFamily.reserve(swapchainIndices.size());

                for (const uint32_t swapchainIndex : swapchainIndices) {
                    const auto vkSwapchain = dynamic_cast<VulkanSwapchain*>(swapchains[swapchainIndex]);
                    vkSwapchains.push_back(vkSwapchain->swapchain);
                    imageIndices.push_back(vkSwapchain->imageIndex);
                    waitSemaphoresForFamily.push_back(presentWaitSemaphores[swapchainIndex]);
                }

                const VkPresentInfoKHR presentInfo{
                    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                    .pNext = nullptr,
                    .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphoresForFamily.size()),
                    .pWaitSemaphores = waitSemaphoresForFamily.data(),
                    .swapchainCount = static_cast<uint32_t>(vkSwapchains.size()),
                    .pSwapchains = vkSwapchains.data(),
                    .pImageIndices = imageIndices.data(),
                    .pResults = results.data()
                };

                Queue& presentQueue = queueFamilies[presentFamily][0];
                presentQueue.submitMutex.lock();
                const VkResult presentResult = vkQueuePresentKHR(presentQueue.queue, &presentInfo);
                presentQueue.submitMutex.unlock();

                if (presentResult != VK_SUCCESS && presentResult != VK_ERROR_OUT_OF_DATE_KHR &&
                    presentResult != VK_SUBOPTIMAL_KHR) {
                    spdlog::error(
                        "vkQueuePresentKHR failed with error: {}",
                        string_VkResult(presentResult)
                    );
                    presentationFailed = true;
                }

                if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
                    resizeRequired = true;
                    for (const uint32_t swapchainIndex : swapchainIndices) {
                        const auto vkSwapchain = dynamic_cast<VulkanSwapchain*>(swapchains[swapchainIndex]);
                        renderingContext->setSurfaceNeedsResize(vkSwapchain->surface, true);
                    }
                }

                for (uint32_t i = 0; i < swapchainIndices.size(); ++i) {
                    const auto vkSwapchain = dynamic_cast<VulkanSwapchain*>(swapchains[swapchainIndices[i]]);
                    vkSwapchain->imageIndex = std::numeric_limits<uint32_t>::max();

                    if (results[i] == VK_ERROR_OUT_OF_DATE_KHR || results[i] == VK_SUBOPTIMAL_KHR) {
                        renderingContext->setSurfaceNeedsResize(vkSwapchain->surface, true);
                        resizeRequired = true;
                    } else if (results[i] != VK_SUCCESS) {
                        presentationFailed = true;
                    }
                }
            }

            if (resizeRequired || presentationFailed)
                return std::unexpected(Error::InitializationFailed);
        }

        return {};
    }

    void VulkanRenderingDeviceDriver::destroyCommandQueue(
        CommandQueue* commandQueue
    ) {
        const auto vkCommandQueue = dynamic_cast<VulkanCommandQueue*>(commandQueue);

        vkQueueWaitIdle(queueFamilies[vkCommandQueue->queueFamily][vkCommandQueue->queueIndex].queue);

        for (const auto& semaphore : vkCommandQueue->imageSemaphores)
            vkDestroySemaphore(device, semaphore, nullptr);

        queueFamilies[vkCommandQueue->queueFamily][vkCommandQueue->queueIndex].virtualCount--;

        delete vkCommandQueue;
    }

    auto VulkanRenderingDeviceDriver::createBuffer(
        const uint64_t size,
        const BufferUsageFlags usage,
        const MemoryAllocationType memoryType
    ) -> std::expected<Buffer*, ResourceCreationError> {
        const VkBufferCreateInfo bufferCreateInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .size = size,
            .usage = toVkBufferUsageFlags(usage),
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr
        };

        VmaAllocationCreateInfo allocationCreateInfo = {
            .flags = 0,
            .usage = VMA_MEMORY_USAGE_UNKNOWN,
            .requiredFlags = 0,
            .preferredFlags = 0,
            .memoryTypeBits = 0,
            .pool = nullptr,
            .pUserData = nullptr,
            .priority = 0.0f,
            .minAlignment = 0
        };

        switch (memoryType) {
            case MemoryAllocationType::Cpu: {
                const bool isSource = usage.contains(BufferUsageBits::CopySource);
                const bool isDestination = usage.contains(BufferUsageBits::CopyDestination);

                if (isSource && !isDestination)
                    allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
                else
                    allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

                allocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

                if (isDestination)
                    allocationCreateInfo.preferredFlags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
                else
                    allocationCreateInfo.preferredFlags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

                allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;

                break;
            }

            case MemoryAllocationType::Gpu: {
                allocationCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

                break;
            }

            default:
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::InvalidDescription,
                        .message = "Buffer memory allocation type is not recognized"
                    }
                };
        }

        auto buffer = new(std::nothrow) VulkanBuffer(usage, size, VK_NULL_HANDLE, nullptr);
        if (buffer == nullptr)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::OutOfHostMemory,
                    .message = "Failed to allocate the Vulkan buffer wrapper"
                }
            };

        VkBuffer vkBuffer;
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo;

        if (const VkResult result = vmaCreateBuffer(
                allocator,
                &bufferCreateInfo,
                &allocationCreateInfo,
                &vkBuffer,
                &allocation,
                &allocationInfo
            );
            result != VK_SUCCESS) {
            delete buffer;
            return std::unexpected{makeResourceCreationError(result, "Vulkan buffer creation")};
        }

        buffer->buffer = vkBuffer;
        buffer->allocation = allocation;

        return buffer;
    }

    void VulkanRenderingDeviceDriver::destroyBuffer(
        Buffer* buffer
    ) {
        const auto o = dynamic_cast<VulkanBuffer*>(buffer);
        vmaDestroyBuffer(allocator, o->buffer, o->allocation);
        delete o;
    }

    auto VulkanRenderingDeviceDriver::createImage(
        const ImageFormat& format,
        const ImageView& view
    ) -> std::expected<Image*, ResourceCreationError> {
        // TODO: Compatible mutable-format views should be added later with first-class image views
        if (view.format != format.format)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::UnsupportedFormat,
                    .message = "Image view format must match the image format until mutable-format views are supported"
                }
            };

        const auto imageType = toVkImageType(format.type);
        if (!imageType)
            return std::unexpected{imageType.error()};

        const auto imageFormat = toVkDataFormat(format.format);
        if (!imageFormat)
            return std::unexpected{imageFormat.error()};

        const auto sampleCount = toVkSampleCountFlagBits(format.samples);
        if (!sampleCount)
            return std::unexpected{sampleCount.error()};

        const auto imageViewType = toVkImageViewType(format.type);
        if (!imageViewType)
            return std::unexpected{imageViewType.error()};

        const auto viewFormat = toVkDataFormat(view.format);
        if (!viewFormat)
            return std::unexpected{viewFormat.error()};

        const auto redSwizzle = toVkComponentSwizzle(view.swizzleRed);
        if (!redSwizzle)
            return std::unexpected{redSwizzle.error()};

        const auto greenSwizzle = toVkComponentSwizzle(view.swizzleGreen);
        if (!greenSwizzle)
            return std::unexpected{greenSwizzle.error()};

        const auto blueSwizzle = toVkComponentSwizzle(view.swizzleBlue);
        if (!blueSwizzle)
            return std::unexpected{blueSwizzle.error()};

        const auto alphaSwizzle = toVkComponentSwizzle(view.swizzleAlpha);
        if (!alphaSwizzle)
            return std::unexpected{alphaSwizzle.error()};

        VkImageCreateInfo imageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = *imageType,
            .format = *imageFormat,
            .extent = {
                .width = format.width,
                .height = format.height,
                .depth = format.depth
            },
            .mipLevels = format.mipmapCount,
            .arrayLayers = format.layerCount,
            .samples = *sampleCount,
            .tiling = format.usage.contains(ImageUsageBits::CpuRead) ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL,
            .usage = 0,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        };

        if (format.type == ImageType::Cube || format.type == ImageType::CubeArray)
            imageCreateInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        if (format.usage.contains(ImageUsageBits::Sampling))
            imageCreateInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

        if (format.usage.contains(ImageUsageBits::Storage) || format.usage.contains(ImageUsageBits::AtomicStorage))
            imageCreateInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;

        if (format.usage.contains(ImageUsageBits::ColorAttachment))
            imageCreateInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        if (format.usage.contains(ImageUsageBits::DepthStencilAttachment))
            imageCreateInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        if (format.usage.contains(ImageUsageBits::InputAttachment))
            imageCreateInfo.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

        if (format.usage.contains(ImageUsageBits::Update))
            imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        if (format.usage.contains(ImageUsageBits::CopySource))
            imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        if (format.usage.contains(ImageUsageBits::CopyDestination))
            imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo allocationCreateInfo{
            .flags = format.usage.contains(ImageUsageBits::CpuRead)
                         ? VmaAllocationCreateFlags{VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT}
                         : VmaAllocationCreateFlags{},
            .usage = VMA_MEMORY_USAGE_AUTO,
            .requiredFlags = 0,
            .preferredFlags = 0,
            .memoryTypeBits = 0,
            .pool = nullptr,
            .pUserData = nullptr,
            .priority = 0.0f,
            .minAlignment = 0
        };

        if (format.usage.contains(ImageUsageBits::TransientAttachment)) {
            constexpr auto transientCompatibleUsages = ImageUsageBits::TransientAttachment |
                ImageUsageBits::ColorAttachment |
                ImageUsageBits::DepthStencilAttachment |
                ImageUsageBits::InputAttachment;

            if ((format.usage.value() & ~transientCompatibleUsages.value()) != 0)
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::UnsupportedUsage,
                        .message = "Transient attachments may only also declare color-attachment, "
                        "depth-stencil-attachment, or input-attachment usage"
                    }
                };

            constexpr auto attachmentUsages = ImageUsageBits::ColorAttachment |
                ImageUsageBits::DepthStencilAttachment |
                ImageUsageBits::InputAttachment;
            if ((format.usage & attachmentUsages).empty())
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::UnsupportedUsage,
                        .message = "Transient-attachment usage requires at least one attachment usage"
                    }
                };

            imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
        }

        const auto validationResult = validateImageFormatSupport(imageCreateInfo);
        if (!validationResult)
            return std::unexpected{std::move(validationResult).error()};

        if (format.usage.contains(ImageUsageBits::TransientAttachment)) {
            uint32_t memoryTypeIndex = 0;
            VmaAllocationCreateInfo lazyMemoryRequirements = allocationCreateInfo;
            lazyMemoryRequirements.usage = VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;

            if (const VkResult result = vmaFindMemoryTypeIndexForImageInfo(
                    allocator,
                    &imageCreateInfo,
                    &lazyMemoryRequirements,
                    &memoryTypeIndex
                );
                result == VK_SUCCESS)
                allocationCreateInfo = lazyMemoryRequirements;
            else
                allocationCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        } else if (format.usage.contains(ImageUsageBits::CpuRead)) {
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
            allocationCreateInfo.requiredFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
            allocationCreateInfo.preferredFlags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        } else {
            allocationCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }

        // TODO: Handle small allocations

        auto image = new(std::nothrow) VulkanImage();
        if (image == nullptr)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::OutOfHostMemory,
                    .message = "Failed to allocate the Vulkan image wrapper"
                }
            };

        VkImage vkImage;
        VmaAllocation allocation;
        if (const VkResult result = vmaCreateImage(
                allocator,
                &imageCreateInfo,
                &allocationCreateInfo,
                &vkImage,
                &allocation,
                nullptr
            );
            result != VK_SUCCESS) {
            delete image;
            return std::unexpected{makeResourceCreationError(result, "Vulkan image creation")};
        }

        const VkImageViewCreateInfo imageViewInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = vkImage,
            .viewType = *imageViewType,
            .format = *viewFormat,
            .components = {
                .r = *redSwizzle,
                .g = *greenSwizzle,
                .b = *blueSwizzle,
                .a = *alphaSwizzle
            },
            .subresourceRange = {
                .aspectMask = toVkImageAspectFlags(getImageAspects(format.format)),
                .baseMipLevel = 0,
                .levelCount = imageCreateInfo.mipLevels,
                .baseArrayLayer = 0,
                .layerCount = imageCreateInfo.arrayLayers
            }
        };

        VkImageView imageView;
        if (const auto e = vkCreateImageView(device, &imageViewInfo, nullptr, &imageView);
            e != VK_SUCCESS) {
            delete image;
            vmaDestroyImage(allocator, vkImage, allocation);
            return std::unexpected{
                makeResourceCreationError(
                    e,
                    "Vulkan image-view creation",
                    ResourceCreationErrorCode::NativeViewCreationFailed
                )
            };
        }

        image->format = format;
        image->view = view;
        image->image = vkImage;
        image->imageView = imageView;
        image->allocation = allocation;
        return image;
    }

    std::byte* VulkanRenderingDeviceDriver::mapImage(
        Image* image
    ) {
        const auto o = dynamic_cast<VulkanImage*>(image);
        std::byte* data;
        vmaMapMemory(allocator, o->allocation, std::bit_cast<void**>(&data));
        return data;
    }

    void VulkanRenderingDeviceDriver::unmapImage(
        Image* image
    ) {
        const auto o = dynamic_cast<VulkanImage*>(image);
        vmaUnmapMemory(allocator, o->allocation);
    }

    void VulkanRenderingDeviceDriver::destroyImage(
        Image* image
    ) {
        const auto o = dynamic_cast<VulkanImage*>(image);
        vkDestroyImageView(device, o->imageView, nullptr);
        vmaDestroyImage(allocator, o->image, o->allocation);
        delete o;
    }

    auto VulkanRenderingDeviceDriver::createSampler(
        SamplerState state
    ) -> std::expected<Sampler*, Error> {
        const VkSamplerCreateInfo samplerInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .magFilter = state.mag == SamplerFilter::Linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST,
            .minFilter = state.min == SamplerFilter::Linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST,
            .mipmapMode = state.mip == SamplerFilter::Linear
                              ? VK_SAMPLER_MIPMAP_MODE_LINEAR
                              : VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .addressModeU = requireVkConversion(toVkSamplerAddressMode(state.u)),
            .addressModeV = requireVkConversion(toVkSamplerAddressMode(state.v)),
            .addressModeW = requireVkConversion(toVkSamplerAddressMode(state.w)),
            .mipLodBias = state.lodBias,
            .anisotropyEnable = state.useAnisotropy && physicalDeviceFeatures.core.features.samplerAnisotropy,
            .maxAnisotropy = state.maxAnisotropy,
            .compareEnable = state.enableCompare,
            .compareOp = static_cast<VkCompareOp>(state.compareOperator),
            .minLod = state.minLod,
            .maxLod = state.maxLod,
            .borderColor = requireVkConversion(toVkBorderColor(state.borderColor)),
            .unnormalizedCoordinates = state.unnormalizedCoordinates ? VK_TRUE : VK_FALSE
        };

        VkSampler sampler;
        if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
            return std::unexpected(Error::InitializationFailed);

        const auto o = new VulkanSampler{};
        o->state = state;
        o->sampler = sampler;
        return o;
    }

    void VulkanRenderingDeviceDriver::destroySampler(
        Sampler* sampler
    ) {
        const auto o = dynamic_cast<VulkanSampler*>(sampler);
        vkDestroySampler(device, o->sampler, nullptr);
        delete o;
    }

    Shader* VulkanRenderingDeviceDriver::createShaderFromSpirv(
        const std::string& name,
        const std::vector<ShaderStageData>& stages
    ) {
        const auto o = new VulkanShader();
        auto shaderCleanup = std::experimental::scope_exit([&] noexcept {
            for (const auto& shaderModule : o->shaderModules)
                vkDestroyShaderModule(device, shaderModule.module, nullptr);

            delete o;
        });

        const auto reflection = reflectShader(stages, o);
        if (!reflection) {
            const auto& reflectionError = reflection.error();
            error<CantCreateError>(
                std::format("Shader '{}' reflection failed: {}", name, reflectionError.detail)
            );
        }

        const auto validateDeviceLimits = [&]() -> std::expected<void, ShaderReflectionError> {
            const auto& limits = physicalDeviceProperties.limits;

            auto limitError = [](
                const ShaderReflectionErrorCode code,
                std::string detail,
                const uint64_t limit,
                const uint64_t actual,
                const std::optional<ShaderStageBits> stage = std::nullopt,
                const ShaderUniform* uniform = nullptr
            ) -> std::expected<void, ShaderReflectionError> {
                return std::unexpected(ShaderReflectionError{
                    .type = code,
                    .stages = stage,
                    .resourceName = {},
                    .set = uniform != nullptr ? std::optional(uniform->set) : std::nullopt,
                    .binding = uniform != nullptr ? std::optional(uniform->binding) : std::nullopt,
                    .expected = limit,
                    .actual = actual,
                    .detail = std::move(detail)
                });
            };

            if (o->pushConstantSize > limits.maxPushConstantsSize)
                return limitError(
                    ShaderReflectionErrorCode::PushConstantLimitExceeded,
                    "Push-constant block exceeds the device limit",
                    limits.maxPushConstantsSize,
                    o->pushConstantSize
                );

            struct DescriptorCounts {
                uint64_t samplers = 0;
                uint64_t uniformBuffers = 0;
                uint64_t storageBuffers = 0;
                uint64_t sampledImages = 0;
                uint64_t storageImages = 0;
                uint64_t inputAttachments = 0;
                uint64_t resources = 0;
            };

            auto addDescriptor = [](DescriptorCounts& counts, const ShaderUniform& uniform) {
                switch (uniform.type) {
                    case ShaderUniformType::Sampler:
                        counts.samplers += uniform.count;
                        return;

                    case ShaderUniformType::SampledImage:
                    case ShaderUniformType::UniformTexelBuffer:
                        counts.sampledImages += uniform.count;
                        break;

                    case ShaderUniformType::CombinedImageSampler:
                        counts.samplers += uniform.count;
                        counts.sampledImages += uniform.count;
                        break;

                    case ShaderUniformType::StorageImage:
                    case ShaderUniformType::StorageTexelBuffer:
                        counts.storageImages += uniform.count;
                        break;

                    case ShaderUniformType::UniformBuffer:
                        counts.uniformBuffers += uniform.count;
                        break;

                    case ShaderUniformType::StorageBuffer:
                        counts.storageBuffers += uniform.count;
                        break;

                    case ShaderUniformType::InputAttachment:
                        counts.inputAttachments += uniform.count;
                        break;
                }

                counts.resources += uniform.count;
            };

            DescriptorCounts totalCounts{};
            for (const ShaderUniform& uniform : o->uniformSets) {
                if (uniform.set >= limits.maxBoundDescriptorSets)
                    return limitError(
                        ShaderReflectionErrorCode::DescriptorSetLimitExceeded,
                        "Descriptor set index exceeds maxBoundDescriptorSets",
                        limits.maxBoundDescriptorSets,
                        static_cast<uint64_t>(uniform.set) + 1,
                        std::nullopt,
                        &uniform
                    );
                addDescriptor(totalCounts, uniform);
            }

            auto validateCounts = [&](
                const DescriptorCounts& counts,
                const bool perStage,
                const std::optional<ShaderStageBits> stage = std::nullopt
            ) -> std::expected<void, ShaderReflectionError> {
                const auto validate = [&](const uint64_t actual, const uint64_t limit, const char* name)
                    -> std::expected<void, ShaderReflectionError> {
                    if (actual <= limit)
                        return {};

                    return limitError(
                        ShaderReflectionErrorCode::DescriptorTypeLimitExceeded,
                        std::format("{} descriptor count exceeds the device limit", name),
                        limit,
                        actual,
                        stage
                    );
                };

                if (auto result = validate(
                        counts.samplers,
                        perStage
                            ? limits.maxPerStageDescriptorSamplers
                            : limits.maxDescriptorSetSamplers,
                        "Sampler"
                    );
                    !result)
                    return result;

                if (auto result = validate(
                        counts.uniformBuffers,
                        perStage
                            ? limits.maxPerStageDescriptorUniformBuffers
                            : limits.maxDescriptorSetUniformBuffers,
                        "Uniform-buffer"
                    );
                    !result)
                    return result;

                if (auto result = validate(
                        counts.storageBuffers,
                        perStage
                            ? limits.maxPerStageDescriptorStorageBuffers
                            : limits.maxDescriptorSetStorageBuffers,
                        "Storage-buffer"
                    );
                    !result)
                    return result;

                if (auto result = validate(
                        counts.sampledImages,
                        perStage
                            ? limits.maxPerStageDescriptorSampledImages
                            : limits.maxDescriptorSetSampledImages,
                        "Sampled-image"
                    );
                    !result)
                    return result;

                if (auto result = validate(
                        counts.storageImages,
                        perStage
                            ? limits.maxPerStageDescriptorStorageImages
                            : limits.maxDescriptorSetStorageImages,
                        "Storage-image"
                    );
                    !result)
                    return result;

                if (auto result = validate(
                        counts.inputAttachments,
                        perStage
                            ? limits.maxPerStageDescriptorInputAttachments
                            : limits.maxDescriptorSetInputAttachments,
                        "Input-attachment"
                    );
                    !result)
                    return result;

                if (perStage)
                    if (auto result = validate(
                            counts.resources,
                            limits.maxPerStageResources,
                            "Per-stage resource"
                        );
                        !result)
                        return result;

                return {};
            };

            if (auto result = validateCounts(totalCounts, false); !result)
                return result;

            constexpr std::array shaderStages{
                ShaderStageBits::Vertex,
                ShaderStageBits::Fragment,
                ShaderStageBits::TesselationControl,
                ShaderStageBits::TesselationEvaluation,
                ShaderStageBits::Compute,
                ShaderStageBits::Geometry
            };
            for (const ShaderStageBits stage : shaderStages) {
                if (!o->stages.contains(stage))
                    continue;

                DescriptorCounts stageCounts{};
                for (const ShaderUniform& uniform : o->uniformSets) {
                    if (uniform.stages.contains(stage))
                        addDescriptor(stageCounts, uniform);
                }

                if (auto result = validateCounts(stageCounts, true, stage); !result)
                    return result;
            }

            return {};
        };

        if (const auto limitsValidation = validateDeviceLimits(); !limitsValidation) {
            const auto& validationError = limitsValidation.error();
            error<CantCreateError>(
                std::format("Shader '{}' exceeds Vulkan device limits: {}", name, validationError.detail)
            );
        }

        o->name = name;

        o->shaderModules.reserve(stages.size());
        for (const auto& stageData : stages) {
            VulkanShaderModule shaderModule{
                .stage = static_cast<VkShaderStageFlagBits>(toVkShaderStageFlags(stageData.stage)),
                .module = VK_NULL_HANDLE,
                .entryPoint = stageData.entryPoint
            };

            const VkShaderModuleCreateInfo shaderModuleInfo{
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .codeSize = stageData.spirv.size(),
                .pCode = std::bit_cast<const uint32_t*>(stageData.spirv.data())
            };

            if (vkCreateShaderModule(device, &shaderModuleInfo, nullptr, &shaderModule.module) != VK_SUCCESS) {
                error<CantCreateError>("Call to vkCreateShaderModule failed.");
            }

            o->shaderModules.push_back(std::move(shaderModule));
        }

        shaderCleanup.release();
        return o;
    }

    void VulkanRenderingDeviceDriver::destroyShaderModules(
        Shader* shader
    ) {
        const auto o = dynamic_cast<VulkanShader*>(shader);
        for (const auto& shaderModule : o->shaderModules)
            vkDestroyShaderModule(device, shaderModule.module, nullptr);
        o->shaderModules.clear();
    }

    void VulkanRenderingDeviceDriver::destroyShader(
        Shader* shader
    ) {
        const auto o = dynamic_cast<VulkanShader*>(shader);

        destroyShaderModules(o);

        delete o;
    }

    auto VulkanRenderingDeviceDriver::createPipelineLayout(
        const PipelineLayoutDescription& description
    ) -> std::expected<PipelineLayout*, ResourceCreationError> try {
        for (size_t rangeIndex = 0; rangeIndex < description.pushConstantRanges.size(); ++rangeIndex) {
            const auto& range = description.pushConstantRanges[rangeIndex];
            const auto context = std::format("Push-constant range {}", rangeIndex);

            const uint64_t rangeEnd = static_cast<uint64_t>(range.offset) + range.size;
            if (rangeEnd > physicalDeviceProperties.limits.maxPushConstantsSize)
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::ExceedsDeviceLimits,
                        .message = "Pipeline layout push-constant range exceeds the device size limit",
                        .limitViolation = ResourceCreationLimitViolation{
                            .limit = "maxPushConstantsSize",
                            .requested = rangeEnd,
                            .supported = physicalDeviceProperties.limits.maxPushConstantsSize
                        },
                        .details = {
                            context,
                            std::format("Byte range: [{}, {})", range.offset, rangeEnd)
                        }
                    }
                };
        }

        PipelineLayoutDescription storedDescription = description;
        std::ranges::sort(
            storedDescription.descriptorSets,
            {},
            &DescriptorSetLayoutDescription::set
        );
        for (auto& descriptorSet : storedDescription.descriptorSets)
            std::ranges::sort(
                descriptorSet.bindings,
                {},
                &DescriptorBindingLayout::binding
            );

        const size_t nativeSetLayoutCount = storedDescription.descriptorSets.empty()
                                                ? 0
                                                : static_cast<size_t>(storedDescription.descriptorSets.back().set) + 1;
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts(nativeSetLayoutCount, VK_NULL_HANDLE);

        auto descriptorSetLayoutCleanup = std::experimental::scope_exit([&] noexcept {
            for (const auto descriptorSetLayout : descriptorSetLayouts)
                if (descriptorSetLayout != VK_NULL_HANDLE)
                    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        });

        size_t describedSetIndex = 0;
        for (size_t setIndex = 0; setIndex < nativeSetLayoutCount; ++setIndex) {
            const auto setNumber = static_cast<uint32_t>(setIndex);
            const DescriptorSetLayoutDescription* descriptorSet = nullptr;
            if (describedSetIndex < storedDescription.descriptorSets.size() &&
                storedDescription.descriptorSets[describedSetIndex].set == setNumber) {
                descriptorSet = &storedDescription.descriptorSets[describedSetIndex];
                ++describedSetIndex;
            }

            std::vector<VkDescriptorSetLayoutBinding> bindings{};
            std::vector<std::vector<VkSampler>> immutableSamplerStorage{};
            if (descriptorSet != nullptr) {
                bindings.reserve(descriptorSet->bindings.size());
                immutableSamplerStorage.resize(descriptorSet->bindings.size());
            }

            for (size_t bindingIndex = 0;
                 descriptorSet != nullptr && bindingIndex < descriptorSet->bindings.size();
                 ++bindingIndex) {
                const auto& binding = descriptorSet->bindings[bindingIndex];
                auto& samplers = immutableSamplerStorage[bindingIndex];
                samplers.reserve(binding.immutableSamplers.size());
                for (const auto sampler : binding.immutableSamplers) {
                    if (sampler == nullptr)
                        return std::unexpected{
                            ResourceCreationError{
                                .code = ResourceCreationErrorCode::InvalidDescription,
                                .message = "Pipeline layout contains a null immutable sampler",
                                .details = {
                                    std::format(
                                        "Descriptor set {}, binding {}",
                                        descriptorSet->set,
                                        binding.binding
                                    )
                                }
                            }
                        };

                    const auto vkSampler = dynamic_cast<const VulkanSampler*>(sampler);
                    if (vkSampler == nullptr) {
                        return std::unexpected{
                            ResourceCreationError{
                                .code = ResourceCreationErrorCode::InvalidDescription,
                                .message =
                                "Pipeline layout contains an immutable sampler from an incompatible rendering backend",
                                .details = {
                                    std::format(
                                        "Descriptor set {}, binding {}",
                                        descriptorSet->set,
                                        binding.binding
                                    )
                                }
                            }
                        };
                    }

                    samplers.push_back(vkSampler->sampler);
                }

                auto type = toVkDescriptorType(binding.type);
                if (!type) {
                    auto error = std::move(type).error();
                    error.details.push_back(
                        std::format(
                            "While converting descriptor set {}, binding {}",
                            descriptorSet->set,
                            binding.binding
                        )
                    );
                    return std::unexpected{std::move(error)};
                }

                bindings.push_back(VkDescriptorSetLayoutBinding{
                    .binding = binding.binding,
                    .descriptorType = *type,
                    .descriptorCount = binding.count,
                    .stageFlags = toVkShaderStageFlags(binding.stages),
                    .pImmutableSamplers = samplers.empty() ? nullptr : samplers.data()
                });
            }

            const VkDescriptorSetLayoutCreateInfo info{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .bindingCount = static_cast<uint32_t>(bindings.size()),
                .pBindings = bindings.empty() ? nullptr : bindings.data()
            };

            VkDescriptorSetLayout layout = VK_NULL_HANDLE;
            const auto result = vkCreateDescriptorSetLayout(device, &info, nullptr, &layout);
            if (result != VK_SUCCESS) {
                auto error = makeResourceCreationError(result, "vkCreateDescriptorSetLayout");
                error.details.push_back(
                    descriptorSet != nullptr
                        ? std::format(
                            "Descriptor set {} contains {} bindings",
                            descriptorSet->set,
                            descriptorSet->bindings.size()
                        )
                        : std::format("Descriptor set {} is an implicit empty layout", setNumber)
                );
                return std::unexpected{std::move(error)};
            }

            descriptorSetLayouts[setIndex] = layout;
        }

        std::vector<VkPushConstantRange> pushConstantRanges{};
        pushConstantRanges.reserve(description.pushConstantRanges.size());
        for (const auto& pushConstantRange : description.pushConstantRanges) {
            pushConstantRanges.push_back(VkPushConstantRange{
                .stageFlags = toVkShaderStageFlags(pushConstantRange.stages),
                .offset = pushConstantRange.offset,
                .size = pushConstantRange.size
            });
        }

        const VkPipelineLayoutCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
            .pSetLayouts = descriptorSetLayouts.empty() ? nullptr : descriptorSetLayouts.data(),
            .pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size()),
            .pPushConstantRanges = pushConstantRanges.empty() ? nullptr : pushConstantRanges.data()
        };

        VkPipelineLayout layout = VK_NULL_HANDLE;
        const auto result = vkCreatePipelineLayout(device, &info, nullptr, &layout);
        if (result != VK_SUCCESS) {
            auto error = makeResourceCreationError(result, "vkCreatePipelineLayout");
            error.details.push_back(
                std::format(
                    "Pipeline layout contains {} descriptor-set layouts and {} push-constant ranges",
                    descriptorSetLayouts.size(),
                    description.pushConstantRanges.size()
                )
            );
            return std::unexpected{std::move(error)};
        }

        auto pipelineLayoutCleanup = std::experimental::scope_exit([&] noexcept {
            vkDestroyPipelineLayout(device, layout, nullptr);
        });

        auto pipelineLayout = new(std::nothrow) VulkanPipelineLayout{
            std::move(storedDescription),
            layout,
            std::move(descriptorSetLayouts)
        };
        if (pipelineLayout == nullptr) {
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::OutOfHostMemory,
                    .message = "Failed to allocate a pipeline-layout wrapper"
                }
            };
        }

        pipelineLayoutCleanup.release();
        descriptorSetLayoutCleanup.release();

        return pipelineLayout;
    } catch (const std::bad_alloc&) {
        return std::unexpected{
            ResourceCreationError{
                .code = ResourceCreationErrorCode::OutOfHostMemory,
                .message = "Failed to allocate temporary storage while creating a Vulkan pipeline layout"
            }
        };
    }

    void VulkanRenderingDeviceDriver::destroyPipelineLayout(PipelineLayout* pipelineLayout) {
        const auto o = dynamic_cast<VulkanPipelineLayout*>(pipelineLayout);

        for (const auto& descriptorSetLayout : o->descriptorSetLayouts)
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        vkDestroyPipelineLayout(device, o->layout, nullptr);
        delete o;
    }

    auto VulkanRenderingDeviceDriver::createGraphicsPipeline(
        const GraphicsPipelineDescription& description
    ) -> std::expected<GraphicsPipeline*, ResourceCreationError> try {
        const auto vkLayout = dynamic_cast<const VulkanPipelineLayout*>(description.layout);
        if (!description.layout || !vkLayout)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = description.layout == nullptr
                                   ? "Graphics pipeline description does not specify a pipeline layout"
                                   : "Graphics pipeline layout belongs to an incompatible rendering backend"
                }
            };

        const auto shader = dynamic_cast<const VulkanShader*>(description.shader);
        if (!description.shader || !shader)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = description.shader == nullptr
                                   ? "Graphics pipeline description does not specify a shader"
                                   : "Graphics pipeline shader belongs to an incompatible rendering backend"
                }
            };

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages{};
        shaderStages.reserve(shader->shaderModules.size());
        bool hasVertexStage = false;
        for (const auto& shaderModule : shader->shaderModules) {
            const VkPipelineShaderStageCreateInfo stage{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = shaderModule.stage,
                .module = shaderModule.module,
                .pName = shaderModule.entryPoint.c_str(),
                .pSpecializationInfo = nullptr
            };

            if (shaderModule.stage == VK_SHADER_STAGE_VERTEX_BIT) {
                hasVertexStage = true;
                shaderStages.push_back(stage);
                continue;
            }

            if (shaderModule.stage == VK_SHADER_STAGE_FRAGMENT_BIT) {
                shaderStages.push_back(stage);
                continue;
            }

            if (shaderModule.stage == VK_SHADER_STAGE_COMPUTE_BIT)
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::InvalidDescription,
                        .message = "Graphics pipeline shader contains a compute stage"
                    }
                };

            if (shaderModule.stage == VK_SHADER_STAGE_GEOMETRY_BIT ||
                shaderModule.stage == VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT ||
                shaderModule.stage == VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::UnsupportedUsage,
                        .message = "Graphics pipeline uses shader stages that are not currently supported"
                    }
                };

            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = "Graphics pipeline contains an unrecognized native shader stage"
                }
            };
        }

        if (!hasVertexStage)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = "Graphics pipeline shader has no live vertex shader module"
                }
            };

        std::vector<VkVertexInputBindingDescription> vertexBindings{};
        vertexBindings.reserve(description.vertexBindings.size());
        for (const auto& binding : description.vertexBindings) {
            vertexBindings.push_back(VkVertexInputBindingDescription{
                .binding = binding.binding,
                .stride = binding.stride,
                .inputRate = toVkInputRate(binding.rate)
            });
        }

        std::vector<VkVertexInputAttributeDescription> vertexAttributes{};
        vertexAttributes.reserve(description.vertexAttributes.size());
        for (const auto& attribute : description.vertexAttributes) {
            auto format = toVkDataFormat(attribute.format);
            if (!format) {
                auto error = std::move(format).error();
                error.details.push_back(
                    std::format(
                        "While converting vertex attribute location {}, binding {}",
                        attribute.location,
                        attribute.binding
                    )
                );

                return std::unexpected{std::move(error)};
            }

            vertexAttributes.push_back(VkVertexInputAttributeDescription{
                .location = attribute.location,
                .binding = attribute.binding,
                .format = *format,
                .offset = attribute.offset
            });
        }

        const VkPipelineVertexInputStateCreateInfo vertexInputState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindings.size()),
            .pVertexBindingDescriptions = vertexBindings.data(),
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size()),
            .pVertexAttributeDescriptions = vertexAttributes.data()
        };

        const VkPipelineRasterizationStateCreateInfo rasterizationState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthClampEnable = description.rasterization.isDepthClampEnabled,
            .rasterizerDiscardEnable = description.rasterization.isRasterizerDiscardEnabled,
            .polygonMode = toVkPolygonMode(description.rasterization.polygonMode),
            .cullMode = toVkCullMode(description.rasterization.cullMode),
            .frontFace = toVkFrontFace(description.rasterization.frontFace),
            .depthBiasEnable = description.rasterization.isDepthBiasEnabled,
            .depthBiasConstantFactor = description.rasterization.depthBiasConstantFactor,
            .depthBiasClamp = description.rasterization.depthBiasClamp,
            .depthBiasSlopeFactor = description.rasterization.depthBiasSlopeFactor,
            .lineWidth = description.rasterization.lineWidth
        };

        auto rasterizationSamples = toVkSampleCountFlagBits(description.multisampling.samples);
        if (!rasterizationSamples) {
            auto error = std::move(rasterizationSamples).error();
            error.details.emplace_back("While converting the graphics pipeline multisample state");

            return std::unexpected{std::move(error)};
        }

        const VkPipelineMultisampleStateCreateInfo multisampleState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .rasterizationSamples = *rasterizationSamples,
            .sampleShadingEnable = description.multisampling.isSampleShadingEnabled,
            .minSampleShading = description.multisampling.minSampleShading,
            .pSampleMask = nullptr,
            .alphaToCoverageEnable = description.multisampling.isAlphaToCoverageEnabled,
            .alphaToOneEnable = description.multisampling.isAlphaToOneEnabled
        };

        const VkPipelineDepthStencilStateCreateInfo depthStencilState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthTestEnable = description.depthStencil.isDepthTestEnabled,
            .depthWriteEnable = description.depthStencil.isDepthWriteEnabled,
            .depthCompareOp = toVkCompareOperator(description.depthStencil.compareOperator),
            .depthBoundsTestEnable = description.depthStencil.isDepthBoundsTestEnabled,
            .stencilTestEnable = description.depthStencil.isStencilTestEnabled,
            .front = {
                .failOp = toVkStencilOperator(description.depthStencil.front.failOperator),
                .passOp = toVkStencilOperator(description.depthStencil.front.passOperator),
                .depthFailOp = toVkStencilOperator(description.depthStencil.front.depthFailOperator),
                .compareOp = toVkCompareOperator(description.depthStencil.front.compareOperator),
                .compareMask = description.depthStencil.front.compareMask,
                .writeMask = description.depthStencil.front.writeMask,
                .reference = description.depthStencil.front.reference
            },
            .back = {
                .failOp = toVkStencilOperator(description.depthStencil.back.failOperator),
                .passOp = toVkStencilOperator(description.depthStencil.back.passOperator),
                .depthFailOp = toVkStencilOperator(description.depthStencil.back.depthFailOperator),
                .compareOp = toVkCompareOperator(description.depthStencil.back.compareOperator),
                .compareMask = description.depthStencil.back.compareMask,
                .writeMask = description.depthStencil.back.writeMask,
                .reference = description.depthStencil.back.reference
            },
            .minDepthBounds = description.depthStencil.minDepthBounds,
            .maxDepthBounds = description.depthStencil.maxDepthBounds
        };

        std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments{};
        colorBlendAttachments.reserve(description.colorBlending.size());
        for (const auto& attachment : description.colorBlending) {
            colorBlendAttachments.push_back(VkPipelineColorBlendAttachmentState{
                .blendEnable = attachment.isEnabled,
                .srcColorBlendFactor = toVkBlendFactor(attachment.sourceColorBlendFactor),
                .dstColorBlendFactor = toVkBlendFactor(attachment.destinationColorBlendFactor),
                .colorBlendOp = toVkBlendOperation(attachment.colorBlendOperation),
                .srcAlphaBlendFactor = toVkBlendFactor(attachment.sourceAlphaBlendFactor),
                .dstAlphaBlendFactor = toVkBlendFactor(attachment.destinationAlphaBlendFactor),
                .alphaBlendOp = toVkBlendOperation(attachment.alphaBlendOperation),
                .colorWriteMask = toVkColorComponentFlags(attachment.colorWriteMask)
            });
        }

        VkPipelineColorBlendStateCreateInfo colorBlendState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_CLEAR,
            .attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size()),
            .pAttachments = colorBlendAttachments.data(),
            .blendConstants = {
                description.blendConstants[0],
                description.blendConstants[1],
                description.blendConstants[2],
                description.blendConstants[3]
            }
        };

        const auto dynamicStates = toVkDynamicStates(description.dynamicStates);
        const VkPipelineDynamicStateCreateInfo dynamicState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
            .pDynamicStates = dynamicStates.data()
        };

        const VkPipelineViewportStateCreateInfo viewportState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .viewportCount = 1,
            .pViewports = nullptr,
            .scissorCount = 1,
            .pScissors = nullptr
        };

        const VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .topology = toVkPrimitiveTopology(description.topology),
            .primitiveRestartEnable = description.isPrimitiveRestartEnabled
        };

        std::vector<VkFormat> colorAttachmentFormats{};
        colorAttachmentFormats.reserve(description.colorFormats.size());
        for (size_t attachmentIndex = 0; attachmentIndex < description.colorFormats.size(); ++attachmentIndex) {
            auto format = toVkDataFormat(description.colorFormats[attachmentIndex]);
            if (!format) {
                auto error = std::move(format).error();
                error.details.push_back(std::format("While converting color attachment {}", attachmentIndex));
                return std::unexpected{std::move(error)};
            }

            colorAttachmentFormats.push_back(*format);
        }

        VkFormat depthAttachmentFormat = VK_FORMAT_UNDEFINED;
        VkFormat stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
        if (description.depthStencilFormat) {
            auto format = toVkDataFormat(*description.depthStencilFormat);
            if (!format) {
                auto error = std::move(format).error();
                error.details.emplace_back("While converting the depth/stencil attachment format");
                return std::unexpected{std::move(error)};
            }

            if (hasDepthAspect(*description.depthStencilFormat))
                depthAttachmentFormat = *format;
            if (hasStencilAspect(*description.depthStencilFormat))
                stencilAttachmentFormat = *format;
        }

        const VkPipelineRenderingCreateInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .pNext = nullptr,
            .viewMask = 0,
            .colorAttachmentCount = static_cast<uint32_t>(colorAttachmentFormats.size()),
            .pColorAttachmentFormats = colorAttachmentFormats.empty() ? nullptr : colorAttachmentFormats.data(),
            .depthAttachmentFormat = depthAttachmentFormat,
            .stencilAttachmentFormat = stencilAttachmentFormat
        };

        const VkGraphicsPipelineCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &renderingInfo,
            .flags = 0,
            .stageCount = static_cast<uint32_t>(shaderStages.size()),
            .pStages = shaderStages.data(),
            .pVertexInputState = &vertexInputState,
            .pInputAssemblyState = &inputAssemblyState,
            .pTessellationState = nullptr,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizationState,
            .pMultisampleState = &multisampleState,
            .pDepthStencilState = &depthStencilState,
            .pColorBlendState = &colorBlendState,
            .pDynamicState = &dynamicState,
            .layout = vkLayout->layout,
            .renderPass = VK_NULL_HANDLE,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = 0
        };

        VkPipeline pipeline = VK_NULL_HANDLE;
        const auto result = vkCreateGraphicsPipelines(
            device,
            nullptr,
            1,
            &info,
            nullptr,
            &pipeline
        );
        if (result != VK_SUCCESS) {
            auto error = makeResourceCreationError(result, "vkCreateGraphicsPipelines");
            error.details.push_back(
                std::format(
                    "Graphics pipeline contains {} vertex bindings, {} vertex attributes, and {} color attachments",
                    description.vertexBindings.size(),
                    description.vertexAttributes.size(),
                    description.colorFormats.size()
                )
            );

            return std::unexpected{std::move(error)};
        }

        auto graphicsPipeline = new(std::nothrow) VulkanGraphicsPipeline{*vkLayout, pipeline};
        if (graphicsPipeline == nullptr) {
            vkDestroyPipeline(device, pipeline, nullptr);

            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::OutOfHostMemory,
                    .message = "Failed to allocate a graphics-pipeline wrapper"
                }
            };
        }

        return graphicsPipeline;
    } catch (const std::bad_alloc&) {
        return std::unexpected{
            ResourceCreationError{
                .code = ResourceCreationErrorCode::OutOfHostMemory,
                .message = "Failed to allocate temporary storage while creating a Vulkan graphics pipeline"
            }
        };
    }

    auto VulkanRenderingDeviceDriver::createComputePipeline(
        const ComputePipelineDescription& description
    ) -> std::expected<ComputePipeline*, ResourceCreationError> try {
        const auto vkLayout = dynamic_cast<const VulkanPipelineLayout*>(description.layout);
        if (!description.layout || !vkLayout)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = description.layout == nullptr
                                   ? "Compute pipeline creation requires a pipeline layout"
                                   : "Compute pipeline layout belongs to an incompatible rendering backend"
                }
            };

        const auto shader = dynamic_cast<const VulkanShader*>(description.shader);
        if (!shader)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = description.shader == nullptr
                                   ? "Compute pipeline creation requires a shader"
                                   : "Compute pipeline shader belongs to an incompatible rendering backend"
                }
            };

        if (shader->stages != ShaderStageFlags{ShaderStageBits::Compute})
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = "Compute pipeline shader must contain exactly one compute stage and no graphics stages"
                }
            };

        if (shader->shaderModules.size() != 1 ||
            shader->shaderModules.front().stage != VK_SHADER_STAGE_COMPUTE_BIT)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = "Compute pipeline shader must have exactly one live compute shader module"
                }
            };

        const auto& computeStage = shader->shaderModules.front();

        const VkPipelineShaderStageCreateInfo computeStageInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = computeStage.stage,
            .module = computeStage.module,
            .pName = computeStage.entryPoint.c_str(),
            .pSpecializationInfo = nullptr
        };

        VkComputePipelineCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = computeStageInfo,
            .layout = vkLayout->layout,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = 0
        };

        VkPipeline pipeline = VK_NULL_HANDLE;
        const auto result = vkCreateComputePipelines(
            device,
            nullptr,
            1,
            &info,
            nullptr,
            &pipeline
        );
        if (result != VK_SUCCESS) {
            auto error = makeResourceCreationError(result, "vkCreateComputePipelines");
            error.details.push_back(
                std::format(
                    "Compute pipeline layout contains {} descriptor sets and {} push-constant ranges",
                    vkLayout->getDescription().descriptorSets.size(),
                    vkLayout->getDescription().pushConstantRanges.size()
                )
            );

            return std::unexpected{std::move(error)};
        }

        auto computePipeline = new(std::nothrow) VulkanComputePipeline{*vkLayout, pipeline};
        if (computePipeline == nullptr) {
            vkDestroyPipeline(device, pipeline, nullptr);

            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::OutOfHostMemory,
                    .message = "Failed to allocate a compute-pipeline wrapper"
                }
            };
        }

        return computePipeline;
    } catch (const std::bad_alloc&) {
        return std::unexpected{
            ResourceCreationError{
                .code = ResourceCreationErrorCode::OutOfHostMemory,
                .message = "Failed to allocate temporary storage while creating a Vulkan compute pipeline"
            }
        };
    }

    void VulkanRenderingDeviceDriver::destroyPipeline(Pipeline* pipeline) {
        if (const auto graphics = dynamic_cast<VulkanGraphicsPipeline*>(pipeline)) {
            vkDestroyPipeline(device, graphics->pipeline, nullptr);
        } else if (const auto compute = dynamic_cast<VulkanComputePipeline*>(pipeline)) {
            vkDestroyPipeline(device, compute->pipeline, nullptr);
        } else {
            DEBUG_ASSERT(false);
            return;
        }

        delete pipeline;
    }

    VkImageSubresourceLayers VulkanRenderingDeviceDriver::_imageSubresourceLayers(
        const ImageSubresourceLayers& layers
    ) {
        return {
            .aspectMask = toVkImageAspectFlags(layers.aspect),
            .mipLevel = layers.mipmap,
            .baseArrayLayer = layers.baseLayer,
            .layerCount = layers.layerCount
        };
    }

    VkBufferImageCopy VulkanRenderingDeviceDriver::_bufferImageCopyRegion(
        const BufferImageCopyRegion& region
    ) {
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

    void VulkanRenderingDeviceDriver::commandBeginRenderPass(
        CommandBuffer* commandBuffer,
        const RenderingInfo& renderingInfo
    ) {
        auto* vkCommandBuffer = dynamic_cast<VulkanCommandBuffer*>(commandBuffer);

        DEBUG_ASSERT(vkCommandBuffer != nullptr);
        DEBUG_ASSERT(renderingInfo.extent.x > 0);
        DEBUG_ASSERT(renderingInfo.extent.y > 0);
        DEBUG_ASSERT(renderingInfo.colorAttachments.size() <= physicalDeviceProperties.limits.maxColorAttachments);

        auto& colorAttachments = vkCommandBuffer->renderingAttachmentScratch;
        colorAttachments.clear();

        for (const auto& attachment : renderingInfo.colorAttachments) {
            DEBUG_ASSERT(attachment.image != nullptr);

            const auto* vkImage = dynamic_cast<VulkanImage*>(attachment.image);

            DEBUG_ASSERT(vkImage != nullptr);

            // TODO: Handle resolve

            colorAttachments.emplace_back(
                VkRenderingAttachmentInfo{
                    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                    .pNext = nullptr,
                    .imageView = vkImage->imageView,
                    .imageLayout = requireVkConversion(toVkImageLayout(attachment.layout)),
                    .resolveMode = VK_RESOLVE_MODE_NONE,
                    .resolveImageView = VK_NULL_HANDLE,
                    .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .loadOp = requireVkConversion(toVkLoadAction(attachment.loadAction)),
                    .storeOp = requireVkConversion(toVkStoreAction(attachment.storeAction)),
                    .clearValue = {
                        .color = {
                            {
                                attachment.clearValue.color.r,
                                attachment.clearValue.color.g,
                                attachment.clearValue.color.b,
                                attachment.clearValue.color.a
                            }
                        }
                    }
                }
            );
        }

        VkRenderingAttachmentInfo depthStencilAttachment{};

        const VkRenderingAttachmentInfo* depthAttachment = nullptr;
        const VkRenderingAttachmentInfo* stencilAttachment = nullptr;

        if (renderingInfo.depthStencilAttachment.has_value()) {
            const auto& attachment = *renderingInfo.depthStencilAttachment;

            DEBUG_ASSERT(attachment.image != nullptr);

            const auto* vkImage = dynamic_cast<VulkanImage*>(attachment.image);

            DEBUG_ASSERT(vkImage != nullptr);

            const auto format = attachment.image->format.format;

            DEBUG_ASSERT(hasDepthAspect(format) || hasStencilAspect(format));

            DEBUG_ASSERT(attachment.resolveImage == nullptr);

            depthStencilAttachment = {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .pNext = nullptr,
                .imageView = vkImage->imageView,
                .imageLayout = requireVkConversion(toVkImageLayout(attachment.layout)),
                .resolveMode = VK_RESOLVE_MODE_NONE,
                .resolveImageView = VK_NULL_HANDLE,
                .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .loadOp = requireVkConversion(toVkLoadAction(attachment.loadAction)),
                .storeOp = requireVkConversion(toVkStoreAction(attachment.storeAction)),
                .clearValue = {
                    .depthStencil = {
                        .depth = attachment.clearValue.depth,
                        .stencil = attachment.clearValue.stencil
                    }
                }
            };

            if (hasDepthAspect(attachment.image->format.format))
                depthAttachment = &depthStencilAttachment;

            if (hasStencilAspect(format))
                stencilAttachment = &depthStencilAttachment;
        }

        const VkRenderingInfo vkRenderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderArea = {
                .offset = {
                    .x = 0,
                    .y = 0
                },
                .extent = {
                    .width = renderingInfo.extent.x,
                    .height = renderingInfo.extent.y
                }
            },
            .layerCount = renderingInfo.layerCount,
            .viewMask = 0,
            .colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size()),
            .pColorAttachments = colorAttachments.data(),
            .pDepthAttachment = depthAttachment,
            .pStencilAttachment = stencilAttachment
        };

        vkCmdBeginRendering(vkCommandBuffer->commandBuffer, &vkRenderingInfo);
    }

    void VulkanRenderingDeviceDriver::commandEndRenderPass(
        CommandBuffer* commandBuffer
    ) {
        vkCmdEndRendering(dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->commandBuffer);
    }

    void VulkanRenderingDeviceDriver::commandSetViewport(
        CommandBuffer* commandBuffer,
        const std::vector<glm::uvec2>& viewports
    ) {
        std::vector<VkViewport> vkViewports{};
        vkViewports.reserve(viewports.size());
        for (const auto& viewport : viewports)
            vkViewports.push_back(
                {
                    .x = 0.0f,
                    .y = 0.0f,
                    .width = static_cast<float>(viewport.x),
                    .height = static_cast<float>(viewport.y),
                    .minDepth = 0.0f,
                    .maxDepth = 1.0f
                }
            );

        vkCmdSetViewport(
            dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->commandBuffer,
            0,
            vkViewports.size(),
            vkViewports.data()
        );
    }

    void VulkanRenderingDeviceDriver::commandSetScissor(
        CommandBuffer* commandBuffer,
        const std::vector<glm::uvec2>& scissors
    ) {
        std::vector<VkRect2D> vkScissors{};
        vkScissors.reserve(scissors.size());
        for (const auto& scissor : scissors)
            vkScissors.push_back(
                {
                    .offset = {
                        .x = 0,
                        .y = 0
                    },
                    .extent = {
                        .width = scissor.x,
                        .height = scissor.y
                    }
                }
            );

        vkCmdSetScissor(
            dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->commandBuffer,
            0,
            vkScissors.size(),
            vkScissors.data()
        );
    }

    void VulkanRenderingDeviceDriver::commandSetBlendConstants(
        CommandBuffer* commandBuffer,
        const glm::vec4 blendConstants
    ) {
        const float constants[4] = {
            blendConstants.r,
            blendConstants.g,
            blendConstants.b,
            blendConstants.a
        };

        vkCmdSetBlendConstants(
            dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->commandBuffer,
            constants
        );
    }

    void VulkanRenderingDeviceDriver::commandBindVertexBuffers(
        CommandBuffer* commandBuffer,
        uint32_t count,
        const std::vector<Buffer*>& buffers,
        const std::vector<uint64_t>& offsets
    ) {
        std::vector<VkBuffer> vkBuffers{};
        vkBuffers.reserve(buffers.size());
        for (const auto& buffer : buffers)
            vkBuffers.push_back(dynamic_cast<VulkanBuffer*>(buffer)->buffer);

        vkCmdBindVertexBuffers(
            dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->commandBuffer,
            0,
            vkBuffers.size(),
            vkBuffers.data(),
            offsets.data()
        );
    }

    void VulkanRenderingDeviceDriver::commandBindIndexBuffers(
        CommandBuffer* commandBuffer,
        Buffer* buffer,
        const IndexFormat format,
        const uint64_t offset
    ) {
        vkCmdBindIndexBuffer(
            dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->commandBuffer,
            dynamic_cast<VulkanBuffer*>(buffer)->buffer,
            offset,
            requireVkConversion(toVkIndexType(format))
        );
    }

    void VulkanRenderingDeviceDriver::commandPipelineBarrier(
        CommandBuffer* commandBuffer,
        const PipelineStageFlags sourceStages,
        const PipelineStageFlags destinationStages,
        const std::vector<MemoryBarrier>& memoryBarriers,
        const std::vector<BufferBarrier>& bufferBarriers,
        const std::vector<ImageBarrier>& imageBarriers
    ) {
        std::vector<VkMemoryBarrier2> vkMemoryBarriers{};
        vkMemoryBarriers.reserve(memoryBarriers.size());
        for (const auto& [sourceAccess, targetAccess] : memoryBarriers) {
            vkMemoryBarriers.push_back(
                {
                    .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
                    .pNext = nullptr,
                    .srcStageMask = toVkPipelineStages(sourceStages),
                    .srcAccessMask = toVkAccessFlags(sourceAccess),
                    .dstStageMask = toVkPipelineStages(destinationStages),
                    .dstAccessMask = toVkAccessFlags(targetAccess)
                }
            );
        }

        std::vector<VkBufferMemoryBarrier2> vkBufferBarriers{};
        vkBufferBarriers.reserve(bufferBarriers.size());
        for (const auto& [buffer, sourceAccess, destinationAccess, offset, size] : bufferBarriers) {
            vkBufferBarriers.push_back(
                {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                    .pNext = nullptr,
                    .srcStageMask = toVkPipelineStages(sourceStages),
                    .srcAccessMask = toVkAccessFlags(sourceAccess),
                    .dstStageMask = toVkPipelineStages(destinationStages),
                    .dstAccessMask = toVkAccessFlags(destinationAccess),
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .buffer = dynamic_cast<VulkanBuffer*>(buffer)->buffer,
                    .offset = offset,
                    .size = size
                }
            );
        }

        std::vector<VkImageMemoryBarrier2> vkImageBarriers{};
        vkImageBarriers.reserve(imageBarriers.size());
        for (const auto& [image, sourceAccess, destinationAccess, oldLayout, newLayout, subresources] :
             imageBarriers) {
            vkImageBarriers.push_back(
                {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .pNext = nullptr,
                    .srcStageMask = toVkPipelineStages(sourceStages),
                    .srcAccessMask = toVkAccessFlags(sourceAccess),
                    .dstStageMask = toVkPipelineStages(destinationStages),
                    .dstAccessMask = toVkAccessFlags(destinationAccess),
                    .oldLayout = requireVkConversion(toVkImageLayout(oldLayout)),
                    .newLayout = requireVkConversion(toVkImageLayout(newLayout)),
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = dynamic_cast<VulkanImage*>(image)->image,
                    .subresourceRange = {
                        .aspectMask = toVkImageAspectFlags(subresources.aspect),
                        .baseMipLevel = subresources.baseMipmap,
                        .levelCount = subresources.mipmapCount,
                        .baseArrayLayer = subresources.baseLayer,
                        .layerCount = subresources.layerCount
                    }
                }
            );
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
            dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->commandBuffer,
            &dependencyInfo
        );
    }

    void VulkanRenderingDeviceDriver::commandClearBuffer(
        CommandBuffer* commandBuffer,
        Buffer* buffer,
        const uint64_t offset,
        const uint64_t size
    ) {
        vkCmdFillBuffer(
            dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->commandBuffer,
            dynamic_cast<VulkanBuffer*>(buffer)->buffer,
            offset,
            size,
            0
        );
    }

    void VulkanRenderingDeviceDriver::commandCopyBuffer(
        CommandBuffer* commandBuffer,
        Buffer* source,
        Buffer* destination,
        const std::vector<BufferCopyRegion>& regions
    ) {
        std::vector<VkBufferCopy> vkRegions{};
        vkRegions.reserve(regions.size());
        for (const auto& [sourceOffset, destinationOffset, size] : regions) {
            vkRegions.push_back(
                {
                    .srcOffset = sourceOffset,
                    .dstOffset = destinationOffset,
                    .size = size
                }
            );
        }

        vkCmdCopyBuffer(
            dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->commandBuffer,
            dynamic_cast<VulkanBuffer*>(source)->buffer,
            dynamic_cast<VulkanBuffer*>(destination)->buffer,
            vkRegions.size(),
            vkRegions.data()
        );
    }

    void VulkanRenderingDeviceDriver::commandCopyImage(
        CommandBuffer* commandBuffer,
        Image* source,
        const ImageLayout sourceLayout,
        Image* destination,
        const ImageLayout destinationLayout,
        const std::vector<ImageCopyRegion>& regions
    ) {
        std::vector<VkImageCopy> vkRegions{};
        vkRegions.reserve(regions.size());
        for (const auto& [sourceSubresources, sourceOffset, destinationSubresources, destinationOffset, size] :
             regions) {
            vkRegions.push_back(
                {
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
                }
            );
        }

        vkCmdCopyImage(
            dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->commandBuffer,
            dynamic_cast<VulkanImage*>(source)->image,
            requireVkConversion(toVkImageLayout(sourceLayout)),
            dynamic_cast<VulkanImage*>(destination)->image,
            requireVkConversion(toVkImageLayout(destinationLayout)),
            vkRegions.size(),
            vkRegions.data()
        );
    }

    void VulkanRenderingDeviceDriver::commandResolveImage(
        CommandBuffer* commandBuffer,
        Image* source,
        const ImageLayout sourceLayout,
        const uint32_t sourceLayer,
        const uint32_t sourceMipmap,
        Image* destination,
        const ImageLayout destinationLayout,
        const uint32_t destinationLayer,
        const uint32_t destinationMipmap
    ) {
        const VkImageResolve region{
            .srcSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = sourceMipmap,
                .baseArrayLayer = sourceLayer,
                .layerCount = 1
            },
            .srcOffset = {
                .x = 0,
                .y = 0,
                .z = 0
            },
            .dstSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = destinationMipmap,
                .baseArrayLayer = destinationLayer,
                .layerCount = 1
            },
            .dstOffset = {
                .x = 0,
                .y = 0,
                .z = 0
            },
            .extent = {
                .width = std::max(1u, source->format.width >> sourceMipmap),
                .height = std::max(1u, source->format.height >> sourceMipmap),
                .depth = std::max(1u, source->format.depth >> sourceMipmap)
            }
        };

        vkCmdResolveImage(
            dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->commandBuffer,
            dynamic_cast<VulkanImage*>(source)->image,
            requireVkConversion(toVkImageLayout(sourceLayout)),
            dynamic_cast<VulkanImage*>(destination)->image,
            requireVkConversion(toVkImageLayout(destinationLayout)),
            1,
            &region
        );
    }

    void VulkanRenderingDeviceDriver::commandClearColorImage(
        CommandBuffer* commandBuffer,
        Image* image,
        const ImageLayout imageLayout,
        const glm::vec4& color,
        const ImageSubresourceRange& subresource
    ) {
        const VkClearColorValue vkColor = {
            {
                color.r,
                color.g,
                color.b,
                color.a
            }
        };
        const VkImageSubresourceRange vkSubresource{
            .aspectMask = toVkImageAspectFlags(subresource.aspect),
            .baseMipLevel = subresource.baseMipmap,
            .levelCount = subresource.mipmapCount,
            .baseArrayLayer = subresource.baseLayer,
            .layerCount = subresource.layerCount
        };

        vkCmdClearColorImage(
            dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->commandBuffer,
            dynamic_cast<VulkanImage*>(image)->image,
            requireVkConversion(toVkImageLayout(imageLayout)),
            &vkColor,
            1,
            &vkSubresource
        );
    }

    void VulkanRenderingDeviceDriver::commandCopyBufferToImage(
        CommandBuffer* commandBuffer,
        Buffer* buffer,
        Image* image,
        const ImageLayout layout,
        const std::vector<BufferImageCopyRegion>& regions
    ) {
        std::vector<VkBufferImageCopy> vkRegions{};
        vkRegions.reserve(regions.size());
        for (const auto& region : regions)
            vkRegions.push_back(_bufferImageCopyRegion(region));

        vkCmdCopyBufferToImage(
            dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->commandBuffer,
            dynamic_cast<VulkanBuffer*>(buffer)->buffer,
            dynamic_cast<VulkanImage*>(image)->image,
            requireVkConversion(toVkImageLayout(layout)),
            vkRegions.size(),
            vkRegions.data()
        );
    }

    void VulkanRenderingDeviceDriver::commandCopyImageToBuffer(
        CommandBuffer* commandBuffer,
        Image* image,
        const ImageLayout layout,
        Buffer* buffer,
        const std::vector<BufferImageCopyRegion>& regions
    ) {
        std::vector<VkBufferImageCopy> vkRegions{};
        vkRegions.reserve(regions.size());
        for (const auto& region : regions)
            vkRegions.push_back(_bufferImageCopyRegion(region));

        vkCmdCopyImageToBuffer(
            dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->commandBuffer,
            dynamic_cast<VulkanImage*>(image)->image,
            requireVkConversion(toVkImageLayout(layout)),
            dynamic_cast<VulkanBuffer*>(buffer)->buffer,
            vkRegions.size(),
            vkRegions.data()
        );
    }

    void VulkanRenderingDeviceDriver::commandBeginLabel(
        CommandBuffer* commandBuffer,
        const std::string& label,
        const glm::vec4& color
    ) {
        const VkDebugUtilsLabelEXT info{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
            .pNext = nullptr,
            .pLabelName = label.c_str(),
            .color = {
                color.r,
                color.g,
                color.b,
                color.a
            }
        };

        vkCmdBeginDebugUtilsLabelEXT(
            dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->commandBuffer,
            &info
        );
    }

    void VulkanRenderingDeviceDriver::commandEndLabel(
        CommandBuffer* commandBuffer
    ) {
        vkCmdEndDebugUtilsLabelEXT(dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->commandBuffer);
    }

    auto VulkanRenderingDeviceDriver::getImageUsageSupportedByFormat(
        const ImageDataFormat format,
        const bool isCpuReadable
    ) const -> std::expected<ImageUsageFlags, ResourceCreationError> {
        const auto vkFormat = toVkDataFormat(format);
        if (!vkFormat)
            return std::unexpected{std::move(vkFormat).error()};

        VkFormatProperties2 properties{
            .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2,
            .pNext = nullptr,
            .formatProperties = {}
        };
        vkGetPhysicalDeviceFormatProperties2(physicalDevice, *vkFormat, &properties);

        const auto& features = isCpuReadable
                                   ? properties.formatProperties.linearTilingFeatures
                                   : properties.formatProperties.optimalTilingFeatures;
        ImageUsageFlags usages{};

        if (features & VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT)
            usages |= ImageUsageBits::Sampling;

        if (features & VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT)
            usages |= ImageUsageBits::ColorAttachment;

        if (features & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            usages |= ImageUsageBits::DepthStencilAttachment;

        if (features & VK_FORMAT_FEATURE_2_STORAGE_IMAGE_BIT)
            usages |= ImageUsageBits::Storage;

        if (features & VK_FORMAT_FEATURE_2_STORAGE_IMAGE_ATOMIC_BIT)
            usages |= ImageUsageBits::AtomicStorage;

        return usages;
    }

    auto VulkanRenderingDeviceDriver::getTexelBufferUsageSupportedByFormat(
        const ImageDataFormat format
    ) const -> std::expected<BufferUsageFlags, ResourceCreationError> {
        const auto vkFormat = toVkDataFormat(format);
        if (!vkFormat)
            return std::unexpected{std::move(vkFormat).error()};

        VkFormatProperties2 properties{
            .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2,
            .pNext = nullptr,
            .formatProperties = {}
        };
        vkGetPhysicalDeviceFormatProperties2(physicalDevice, *vkFormat, &properties);

        const auto features = properties.formatProperties.bufferFeatures;
        BufferUsageFlags usages{};

        if ((features & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT) != 0)
            usages |= BufferUsageBits::UniformTexel;

        if ((features & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT) != 0)
            usages |= BufferUsageBits::StorageTexel;

        return usages;
    }

    auto VulkanRenderingDeviceDriver::validateAttachmentFormatSupport(
        const ImageDataFormat format,
        const ImageUsageBits usage,
        const ImageSamples samples
    ) const -> std::expected<void, ResourceCreationError> {
        const auto vkFormat = toVkDataFormat(format);
        if (!vkFormat)
            return std::unexpected{std::move(vkFormat).error()};

        const auto vkSamples = toVkSampleCountFlagBits(samples);
        if (!vkSamples)
            return std::unexpected{std::move(vkSamples).error()};

        VkImageUsageFlags vkUsage = 0;
        std::string_view attachmentType;
        if (usage == ImageUsageBits::ColorAttachment) {
            vkUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            attachmentType = "color";
        } else if (usage == ImageUsageBits::DepthStencilAttachment) {
            vkUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            attachmentType = "depth/stencil";
        } else {
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = "Attachment format validation requires a color or depth/stencil usage"
                }
            };
        }

        const VkImageCreateInfo imageInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = *vkFormat,
            .extent = {
                .width = 1,
                .height = 1,
                .depth = 1
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = *vkSamples,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = vkUsage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        };

        auto support = validateImageFormatSupport(imageInfo);
        if (!support) {
            auto error = std::move(support).error();
            if (error.code == ResourceCreationErrorCode::UnsupportedFormat)
                error.message = std::format(
                    "Image format does not support use as a {} attachment",
                    attachmentType
                );
            else if (error.code == ResourceCreationErrorCode::UnsupportedSampleCount)
                error.message = std::format(
                    "The rendering device does not support {}x sampling for this {} attachment format",
                    static_cast<uint32_t>(*vkSamples),
                    attachmentType
                );

            error.details.push_back(std::format("Format value: {}", static_cast<uint32_t>(format)));
            return std::unexpected{std::move(error)};
        }

        return {};
    }

    PipelineLayoutLimits VulkanRenderingDeviceDriver::getPipelineLayoutLimits() const {
        const auto& limits = physicalDeviceProperties.limits;
        return PipelineLayoutLimits{
            .maxBoundDescriptorSets = limits.maxBoundDescriptorSets,
            .maxDescriptors = {
                .samplers = limits.maxDescriptorSetSamplers,
                .uniformBuffers = limits.maxDescriptorSetUniformBuffers,
                .storageBuffers = limits.maxDescriptorSetStorageBuffers,
                .sampledImages = limits.maxDescriptorSetSampledImages,
                .storageImages = limits.maxDescriptorSetStorageImages,
                .inputAttachments = limits.maxDescriptorSetInputAttachments
            },
            .maxPerStageDescriptors = {
                .samplers = limits.maxPerStageDescriptorSamplers,
                .uniformBuffers = limits.maxPerStageDescriptorUniformBuffers,
                .storageBuffers = limits.maxPerStageDescriptorStorageBuffers,
                .sampledImages = limits.maxPerStageDescriptorSampledImages,
                .storageImages = limits.maxPerStageDescriptorStorageImages,
                .inputAttachments = limits.maxPerStageDescriptorInputAttachments
            },
            .maxPerStageResources = limits.maxPerStageResources
        };
    }

    uint64_t VulkanRenderingDeviceDriver::getMaxBufferSize() const {
        return maxBufferSize;
    }

    uint32_t VulkanRenderingDeviceDriver::getMaxTexelBufferElements() const {
        return physicalDeviceProperties.limits.maxTexelBufferElements;
    }

    uint32_t VulkanRenderingDeviceDriver::getMaxColorAttachments() const {
        return physicalDeviceProperties.limits.maxColorAttachments;
    }

    uint32_t VulkanRenderingDeviceDriver::getMaxVertexInputBindings() const {
        return physicalDeviceProperties.limits.maxVertexInputBindings;
    }

    uint32_t VulkanRenderingDeviceDriver::getMaxVertexInputAttributes() const {
        return physicalDeviceProperties.limits.maxVertexInputAttributes;
    }

    uint32_t VulkanRenderingDeviceDriver::getMaxVertexInputBindingStride() const {
        return physicalDeviceProperties.limits.maxVertexInputBindingStride;
    }

    uint32_t VulkanRenderingDeviceDriver::getMaxVertexInputAttributeOffset() const {
        return physicalDeviceProperties.limits.maxVertexInputAttributeOffset;
    }

    bool VulkanRenderingDeviceDriver::isVertexInputFormatSupported(
        const ImageDataFormat format
    ) const {
        const auto vkFormat = toVkDataFormat(format);
        if (!vkFormat)
            return false;

        VkFormatProperties properties{};
        vkGetPhysicalDeviceFormatProperties(physicalDevice, *vkFormat, &properties);
        return (properties.bufferFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT) != 0;
    }

    bool VulkanRenderingDeviceDriver::isColorBlendSupported(
        const ImageDataFormat format
    ) const {
        const auto vkFormat = toVkDataFormat(format);
        if (!vkFormat)
            return false;

        VkFormatProperties properties{};
        vkGetPhysicalDeviceFormatProperties(
            physicalDevice,
            *vkFormat,
            &properties
        );

        return properties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT;
    }
}
