project("Vixen Engine Vk")

find_package(Vulkan 1.3 REQUIRED)

find_package(spirv_cross_reflect REQUIRED)

FetchContent_Declare(
        vma
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
        GIT_TAG 2f382df218d7e8516dee3b3caccb819a62b571a2
)
FetchContent_Declare(
        volk
        GIT_REPOSITORY https://github.com/zeux/volk
        GIT_TAG 914ea7426fda7949258647561c0416b6d0b72ea5
)
FetchContent_MakeAvailable(VMA volk)

add_library(
        VkVixen
        SHARED
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
        VkRenderPass.cpp
        VkRenderPass.h
        VkCommandBuffer.cpp
        VkCommandBuffer.h
        VkCommandPool.cpp
        VkCommandPool.h
        VkFence.cpp
        VkFence.h
        VkFramebuffer.cpp
        VkFramebuffer.h
        VkImage.cpp
        VkImage.h
        VkImageView.cpp
        VkImageView.h
        VkSemaphore.cpp
        VkSemaphore.h
)
target_link_libraries(
        VkVixen
        PUBLIC
        Vixen
        Vulkan::Headers
        volk
        GPUOpen::VulkanMemoryAllocator
        spirv-cross-core
        spirv-cross-glsl
        spirv-cross-reflect
        spirv-cross-cpp
        freeimage
)

target_compile_definitions(VkVixen PUBLIC -DVK_NO_PROTOTYPES)

find_package(glslang QUIET)
if (${glslang_FOUND})
    target_link_libraries(
            VkVixen
            PUBLIC
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

if (ENABLE_TESTS)
    add_executable(vktest test/main.cpp)
    target_link_libraries(vktest PUBLIC Vixen VkVixen)
    target_include_directories(vktest PUBLIC ../engine)
endif ()