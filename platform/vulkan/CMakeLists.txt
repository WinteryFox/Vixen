project("Vulkan")

find_package(Vulkan 1.3 REQUIRED)

find_package(spirv_cross_reflect REQUIRED)

add_library(
        VkVixen
        STATIC
        ${CMAKE_SOURCE_DIR}/thirdparty/volk/volk.c
        VulkanApplication.cpp
        window/VulkanWindow.cpp
        device/Instance.cpp
        device/VulkanDevice.cpp
        device/VulkanDevice.h
        buffer/VulkanBuffer.cpp
        buffer/VulkanBuffer.h
        window/VulkanSwapchain.cpp
        window/VulkanSwapchain.h
        pipeline/VulkanPipeline.cpp
        pipeline/VulkanPipeline.h
        shader/VulkanShaderModule.cpp
        shader/VulkanShaderModule.h
        shader/VulkanShaderProgram.cpp
        shader/VulkanShaderProgram.h
        rendering/Renderer.cpp
        rendering/Renderer.h
        pipeline/VulkanPipelineLayout.cpp
        pipeline/VulkanPipelineLayout.h
        commandbuffer/VulkanCommandBuffer.cpp
        commandbuffer/VulkanCommandBuffer.h
        commandbuffer/VulkanCommandPool.cpp
        commandbuffer/VulkanCommandPool.h
        synchronization/VulkanFence.cpp
        synchronization/VulkanFence.h
        image/VulkanImage.cpp
        image/VulkanImage.h
        image/VulkanImageView.cpp
        image/VulkanImageView.h
        synchronization/VulkanSemaphore.cpp
        synchronization/VulkanSemaphore.h
        descriptorset/VulkanDescriptorPoolFixed.cpp
        descriptorset/VulkanDescriptorPoolFixed.h
        descriptorset/VulkanDescriptorSetLayout.cpp
        descriptorset/VulkanDescriptorSetLayout.h
        descriptorset/VulkanDescriptorSet.cpp
        descriptorset/VulkanDescriptorSet.h
        rendering/VulkanMesh.cpp
        rendering/VulkanMesh.h
        descriptorset/VulkanDescriptorPoolExpanding.cpp
        descriptorset/VulkanDescriptorPoolExpanding.h
        material/Material.h
        material/MaterialPass.h
        commandbuffer/CommandBufferLevel.h
        commandbuffer/CommandBufferUsage.h
        exception/VulkanException.h
        FrameData.h
        VulkanContext.cpp
        VulkanContext.h
)
target_link_libraries(
        VkVixen
        PUBLIC
        Vixen
        Vulkan::Headers
        spirv-cross-core
        spirv-cross-glsl
        spirv-cross-reflect
        freeimage
)
target_include_directories(
        VkVixen
        PUBLIC
        ${CMAKE_SOURCE_DIR}/src/engine
        ${CMAKE_SOURCE_DIR}/src/engine/platform/vulkan
)

target_compile_definitions(VkVixen PUBLIC -DVK_NO_PROTOTYPES)

find_package(glslang QUIET)
if (${glslang_FOUND})
    target_link_libraries(
            VkVixen
            PRIVATE
            #           glslang::OSDependent
            glslang::glslang
            #           glslang::MachineIndependent
            #           glslang::GenericCodeGen
            #           glslang::OGLCompiler
            #           glslang::glslangValidator
            #           glslang::spirv-remap
            glslang::glslang-default-resource-limits
            #           glslang::SPVRemapper
            glslang::SPIRV
            #           glslang::HLSL
    )
else ()
    pkg_check_modules(glslang REQUIRED IMPORTED_TARGET glslang)
    target_link_libraries(
            VkVixen
            PUBLIC
            PkgConfig::glslang
            PkgConfig::SPIRV
    )
endif ()

if (WIN32)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
    # TODO: Other platforms
endif ()

target_include_directories(VkVixen PUBLIC thirdparty/volk)

target_link_libraries(
        VkVixen
        PUBLIC
        GPUOpen::VulkanMemoryAllocator
)
