find_package(Vulkan 1.3 REQUIRED)

add_library(
        VkVixen
        STATIC
        image/VulkanImage.h
        VulkanRenderingContextDriver.cpp
        VulkanRenderingContextDriver.h
        VulkanRenderingDeviceDriver.cpp
        VulkanRenderingDeviceDriver.h
        image/VulkanSampler.h
        buffer/VulkanBuffer.h
        VulkanSwapchain.h
        command/VulkanCommandPool.h
        command/VulkanCommandBuffer.h
        shader/VulkanShader.h
        command/VulkanFence.h
        command/VulkanSemaphore.h
        command/VulkanCommandQueue.h
        VulkanSurface.h
        VulkanFramebuffer.h
        DeviceFeatureSupport.h
)
vixen_configure_target(VkVixen)
target_link_libraries(
        VkVixen
        PUBLIC
        Vixen
        Vulkan::Headers
        volk::volk
)
target_include_directories(
        VkVixen
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(VkVixen PUBLIC VK_NO_PROTOTYPES)

if (WIN32)
    target_compile_definitions(VkVixen PUBLIC VK_USE_PLATFORM_WIN32_KHR)
elseif (APPLE)
    target_compile_definitions(VkVixen PUBLIC VK_USE_PLATFORM_METAL_EXT)
endif ()

target_link_libraries(
        VkVixen
        PUBLIC
        GPUOpen::VulkanMemoryAllocator
)
