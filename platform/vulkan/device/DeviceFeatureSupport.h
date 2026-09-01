#pragma once

#include <volk.h>

struct DeviceFeatureSupport {
    VkPhysicalDeviceFeatures2 core{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = nullptr,
        .features{}
    };

    VkPhysicalDeviceVulkan12Features vulkan12{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = nullptr,
        .samplerMirrorClampToEdge{},
        .drawIndirectCount{},
        .storageBuffer8BitAccess{},
        .uniformAndStorageBuffer8BitAccess{},
        .storagePushConstant8{},
        .shaderBufferInt64Atomics{},
        .shaderSharedInt64Atomics{},
        .shaderFloat16{},
        .shaderInt8{},
        .descriptorIndexing{},
        .shaderInputAttachmentArrayDynamicIndexing{},
        .shaderUniformTexelBufferArrayDynamicIndexing{},
        .shaderStorageTexelBufferArrayDynamicIndexing{},
        .shaderUniformBufferArrayNonUniformIndexing{},
        .shaderSampledImageArrayNonUniformIndexing{},
        .shaderStorageBufferArrayNonUniformIndexing{},
        .shaderStorageImageArrayNonUniformIndexing{},
        .shaderInputAttachmentArrayNonUniformIndexing{},
        .shaderUniformTexelBufferArrayNonUniformIndexing{},
        .shaderStorageTexelBufferArrayNonUniformIndexing{},
        .descriptorBindingUniformBufferUpdateAfterBind{},
        .descriptorBindingSampledImageUpdateAfterBind{},
        .descriptorBindingStorageImageUpdateAfterBind{},
        .descriptorBindingStorageBufferUpdateAfterBind{},
        .descriptorBindingUniformTexelBufferUpdateAfterBind{},
        .descriptorBindingStorageTexelBufferUpdateAfterBind{},
        .descriptorBindingUpdateUnusedWhilePending{},
        .descriptorBindingPartiallyBound{},
        .descriptorBindingVariableDescriptorCount{},
        .runtimeDescriptorArray{},
        .samplerFilterMinmax{},
        .scalarBlockLayout{},
        .imagelessFramebuffer{},
        .uniformBufferStandardLayout{},
        .shaderSubgroupExtendedTypes{},
        .separateDepthStencilLayouts{},
        .hostQueryReset{},
        .timelineSemaphore{},
        .bufferDeviceAddress{},
        .bufferDeviceAddressCaptureReplay{},
        .bufferDeviceAddressMultiDevice{},
        .vulkanMemoryModel{},
        .vulkanMemoryModelDeviceScope{},
        .vulkanMemoryModelAvailabilityVisibilityChains{},
        .shaderOutputViewportIndex{},
        .shaderOutputLayer{},
        .subgroupBroadcastDynamicId{}
    };

    VkPhysicalDeviceVulkan13Features vulkan13{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = nullptr,
        .robustImageAccess{},
        .inlineUniformBlock{},
        .descriptorBindingInlineUniformBlockUpdateAfterBind{},
        .pipelineCreationCacheControl{},
        .privateData{},
        .shaderDemoteToHelperInvocation{},
        .shaderTerminateInvocation{},
        .subgroupSizeControl{},
        .computeFullSubgroups{},
        .synchronization2{},
        .textureCompressionASTC_HDR{},
        .shaderZeroInitializeWorkgroupMemory{},
        .dynamicRendering{},
        .shaderIntegerDotProduct{},
        .maintenance4{}
    };
};
