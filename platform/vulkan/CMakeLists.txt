find_package(Vulkan 1.3 REQUIRED)

add_library(
        VixenVulkan
        STATIC
        context/VulkanRenderingContextDriver.cpp
        device/VulkanRenderingDeviceDriver.cpp
        pipeline/VulkanGraphicsPipeline.cpp
        pipeline/VulkanComputePipeline.cpp
        pipeline/VulkanPipelineLayout.cpp
)
target_sources(
        VixenVulkan
        PUBLIC
        FILE_SET public_headers
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        buffer/VulkanBuffer.h
        command/CommandBufferLevel.h
        command/CommandBufferUsage.h
        command/VulkanCommandBuffer.h
        command/VulkanCommandPool.h
        command/VulkanCommandQueue.h
        command/VulkanFence.h
        command/VulkanSemaphore.h
        context/VulkanRenderingContextDriver.h
        device/DeviceFeatureSupport.h
        device/VulkanRenderingDeviceDriver.h
        image/VulkanImage.h
        image/VulkanSampler.h
        presentation/VulkanFramebuffer.h
        presentation/VulkanSurface.h
        presentation/VulkanSwapchain.h
        shader/VulkanShader.h
        Vulkan.h
        pipeline/VulkanGraphicsPipeline.h
        pipeline/VulkanPipelineLayout.h
        pipeline/VulkanComputePipeline.h
)
vixen_configure_target(VixenVulkan)
target_link_libraries(
        VixenVulkan
        PUBLIC
        VixenCore
        Vulkan::Headers
        volk::volk
)
target_include_directories(
        VixenVulkan
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(VixenVulkan PUBLIC VK_NO_PROTOTYPES)

if (WIN32)
    target_compile_definitions(VixenVulkan PUBLIC VK_USE_PLATFORM_WIN32_KHR)
elseif (APPLE)
    target_compile_definitions(VixenVulkan PUBLIC VK_USE_PLATFORM_METAL_EXT)
endif ()

target_link_libraries(
        VixenVulkan
        PUBLIC
        GPUOpen::VulkanMemoryAllocator
)
