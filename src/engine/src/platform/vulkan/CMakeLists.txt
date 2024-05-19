project("Vixen Engine Vk")

find_package(Vulkan 1.3 REQUIRED OPTIONAL_COMPONENTS volk)

find_package(spirv_cross_reflect REQUIRED)

add_library(
        VkVixen
        STATIC
        VulkanApplication.cpp
        VulkanWindow.cpp
        Instance.cpp
        VulkanDevice.cpp
        VulkanDevice.h
        buffer/VulkanBuffer.cpp
        buffer/VulkanBuffer.h
        VulkanSwapchain.cpp
        VulkanSwapchain.h
        pipeline/VulkanPipeline.cpp
        pipeline/VulkanPipeline.h
        shader/VulkanShaderModule.cpp
        shader/VulkanShaderModule.h
        shader/VulkanShaderProgram.cpp
        shader/VulkanShaderProgram.h
        Renderer.cpp
        Renderer.h
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
        VulkanMesh.cpp
        VulkanMesh.h
        descriptorset/VulkanDescriptorPoolExpanding.cpp
        descriptorset/VulkanDescriptorPoolExpanding.h
        material/Material.h
        material/MaterialPass.h
        commandbuffer/CommandBufferLevel.h
        commandbuffer/CommandBufferUsage.h
        exception/VulkanException.h
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
        .
        ../..
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
            PRIVATE
            PkgConfig::glslang
            PkgConfig::SPIRV
    )
endif ()

message("Using volk included with Vulkan SDK")
target_link_libraries(VkVixen PRIVATE Vulkan::volk)

find_path(VMA_PATH NAMES vk_mem_alloc.h HINTS ${Vulkan_INCLUDE_DIRS} PATH_SUFFIXES vma)
target_include_directories(VkVixen PRIVATE ${VMA_PATH})
