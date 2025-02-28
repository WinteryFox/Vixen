find_package(Vulkan 1.3 REQUIRED)

add_library(
        VkVixen
        STATIC
        image/VulkanImage.h
        VulkanRenderingContext.cpp
        VulkanRenderingContext.h
        VulkanRenderingDevice.cpp
        VulkanRenderingDevice.h
        image/VulkanSampler.h
        buffer/VulkanBuffer.h
        VulkanSwapchain.h
        GraphicsCard.h
        command/VulkanCommandPool.h
        command/VulkanCommandBuffer.h
        shader/VulkanShader.h
        command/VulkanFence.h
        command/VulkanSemaphore.h
        command/VulkanCommandQueue.h
        VulkanSurface.h
        VulkanFramebuffer.h
)
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
        ${CMAKE_SOURCE_DIR}/src/engine
        ${CMAKE_SOURCE_DIR}/src/engine/platform/vulkan
)

target_compile_definitions(VkVixen PUBLIC VK_NO_PROTOTYPES)

if (WIN32)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
    # TODO: Other platforms
endif ()

target_link_libraries(
        VkVixen
        PUBLIC
        GPUOpen::VulkanMemoryAllocator
)
