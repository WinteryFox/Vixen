#define VMA_IMPLEMENTATION
#include "VulkanRenderingDevice.h"

#include <vk_mem_alloc.h>
#include <Vulkan.h>
#include <spirv_reflect.hpp>
#include <spirv_cross.hpp>

#include "VulkanRenderingContext.h"
#include "buffer/VulkanBuffer.h"
#include "command/VulkanCommandBuffer.h"
#include "command/VulkanCommandPool.h"
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
            .vulkanApiVersion = VK_API_VERSION_1_3,
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
        o->buffer = commandBuffer;

        return o;
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
                imageCreateInfo.usage &= (VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                                          | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                                          VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
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
                                                                  ? VK_IMAGE_ASPECT_DEPTH_BIT
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

        const auto o = new VulkanImage{};
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

        for (const auto &[stage, spirv]: stages) {
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

            constexpr VkPipelineLayoutCreateInfo pipelineLayoutInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .setLayoutCount = 0,
                .pSetLayouts = nullptr,
                .pushConstantRangeCount = 0,
                .pPushConstantRanges = nullptr
            };

            VkPipelineLayout pipelineLayout;
            ASSERT_THROW(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) == VK_SUCCESS,
                         CantCreateError,
                         "Call to vkCreatePipelineLayout failed.");
            o->pipelineLayout = pipelineLayout;

            std::vector<VkDescriptorSetLayoutBinding> layoutBindings{};
            layoutBindings.reserve(o->uniformSets.size());
            for (const auto &uniformBuffer: o->uniformSets) {
                const VkDescriptorSetLayoutBinding layoutBinding = {
                    .binding = uniformBuffer.binding,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = 0,
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
            o->descriptorSetLayouts = {descriptorSetLayout};

            VkShaderStageFlagBits stageFlag = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
            switch (stage) {
                case ShaderStage::Vertex:
                    stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
                    break;

                case ShaderStage::Fragment:
                    stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;
                    break;

                case ShaderStage::TesselationControl:
                    stageFlag = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
                    break;

                case ShaderStage::TesselationEvaluation:
                    stageFlag = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                    break;

                case ShaderStage::Compute:
                    stageFlag = VK_SHADER_STAGE_COMPUTE_BIT;
                    break;

                case ShaderStage::Geometry:
                    stageFlag = VK_SHADER_STAGE_GEOMETRY_BIT;
                    break;
            }

            o->shaderStageInfos.push_back({
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = stageFlag,
                .module = module,
                .pName = "main",
                .pSpecializationInfo = nullptr
            });
        }

        return o;
    }

    void VulkanRenderingDevice::destroyShaderModules(Shader *shader) {
        for (const auto o = reinterpret_cast<VulkanShader *>(shader);
             const auto &module: o->shaderStageInfos) {
            vkDestroyShaderModule(device, module.module, nullptr);
        }
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
