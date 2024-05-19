project("Vixen Engine Vk")

find_package(Vulkan 1.3 REQUIRED OPTIONAL_COMPONENTS volk)

find_package(spirv_cross_reflect REQUIRED)

add_library(
        VkVixen
        STATIC
        src/VulkanApplication.cpp
        src/VulkanWindow.cpp
        src/Instance.cpp
        src/VulkanDevice.cpp
        src/VulkanDevice.h
        src/buffer/VulkanBuffer.cpp
        src/buffer/VulkanBuffer.h
        src/VulkanSwapchain.cpp
        src/VulkanSwapchain.h
        src/pipeline/VulkanPipeline.cpp
        src/pipeline/VulkanPipeline.h
        src/shader/VulkanShaderModule.cpp
        src/shader/VulkanShaderModule.h
        src/shader/VulkanShaderProgram.cpp
        src/shader/VulkanShaderProgram.h
        src/Renderer.cpp
        src/Renderer.h
        src/pipeline/VulkanPipelineLayout.cpp
        src/pipeline/VulkanPipelineLayout.h
        src/commandbuffer/VulkanCommandBuffer.cpp
        src/commandbuffer/VulkanCommandBuffer.h
        src/commandbuffer/VulkanCommandPool.cpp
        src/commandbuffer/VulkanCommandPool.h
        src/synchronization/VulkanFence.cpp
        src/synchronization/VulkanFence.h
        src/image/VulkanImage.cpp
        src/image/VulkanImage.h
        src/image/VulkanImageView.cpp
        src/image/VulkanImageView.h
        src/synchronization/VulkanSemaphore.cpp
        src/synchronization/VulkanSemaphore.h
        src/descriptorset/VulkanDescriptorPoolFixed.cpp
        src/descriptorset/VulkanDescriptorPoolFixed.h
        src/descriptorset/VulkanDescriptorSetLayout.cpp
        src/descriptorset/VulkanDescriptorSetLayout.h
        src/descriptorset/VulkanDescriptorSet.cpp
        src/descriptorset/VulkanDescriptorSet.h
        src/VulkanMesh.cpp
        src/VulkanMesh.h
        src/descriptorset/VulkanDescriptorPoolExpanding.cpp
        src/descriptorset/VulkanDescriptorPoolExpanding.h
        src/exception/OutOfPoolMemoryException.h
        src/material/Material.h
        src/material/MaterialPass.h
        src/commandbuffer/CommandBufferLevel.h
        src/commandbuffer/CommandBufferUsage.h
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
        ../../
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

if (ENABLE_TESTS)
    add_executable(vktest src/test/main.cpp)
    target_link_libraries(vktest PRIVATE VkVixen)
    target_include_directories(vktest PRIVATE ../engine/src)
endif ()
