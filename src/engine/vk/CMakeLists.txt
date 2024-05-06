project("Vixen Engine Vk")

find_package(Vulkan 1.3 REQUIRED OPTIONAL_COMPONENTS volk)

find_package(spirv_cross_reflect REQUIRED)

add_library(
        VkVixen
        STATIC
        VkVixen.cpp
        VkWindow.cpp
        Instance.cpp
        Device.cpp
        Device.h
        VkBuffer.cpp
        Swapchain.cpp
        Swapchain.h
        VkPipeline.cpp
        VkPipeline.h
        VkShaderModule.cpp
        VkShaderModule.h
        VkShaderProgram.cpp
        VkShaderProgram.h
        VkRenderer.cpp
        VkRenderer.h
        VkPipelineLayout.cpp
        VkPipelineLayout.h
        VkCommandBuffer.cpp
        VkCommandBuffer.h
        VkCommandPool.cpp
        VkCommandPool.h
        VkFence.cpp
        VkFence.h
        VkImage.cpp
        VkImage.h
        VkImageView.cpp
        VkImageView.h
        VkSemaphore.cpp
        VkSemaphore.h
        VkDescriptorPoolFixed.cpp
        VkDescriptorPoolFixed.h
        VkDescriptorSetLayout.cpp
        VkDescriptorSetLayout.h
        VkDescriptorSet.cpp
        VkDescriptorSet.h
        VkMesh.cpp
        VkMesh.h
        VkDescriptorPoolExpanding.cpp
        VkDescriptorPoolExpanding.h
        exception/OutOfPoolMemoryException.h
        material/MaterialPipeline.h
        material/Material.h
        material/MaterialPass.h
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
    add_executable(vktest test/main.cpp)
    target_link_libraries(vktest PRIVATE VkVixen)
    target_include_directories(vktest PRIVATE ../engine)
endif ()
