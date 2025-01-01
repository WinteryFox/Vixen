find_package(Vulkan 1.3 REQUIRED)

add_library(
        volk
        STATIC
        ${CMAKE_SOURCE_DIR}/thirdparty/volk/volk.c
)

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
)
target_link_libraries(
        VkVixen
        PUBLIC
        Vixen
        Vulkan::Headers
        freeimage
        volk
)
target_include_directories(
        VkVixen
        PUBLIC
        ${CMAKE_SOURCE_DIR}/src/engine
        ${CMAKE_SOURCE_DIR}/src/engine/platform/vulkan
)

target_compile_definitions(VkVixen PUBLIC VK_NO_PROTOTYPES)

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

if (WIN32)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
    # TODO: Other platforms
endif ()

target_include_directories(
        VkVixen
        PUBLIC
        ${CMAKE_SOURCE_DIR}/thirdparty/volk
        ${CMAKE_SOURCE_DIR}/thirdparty/VulkanMemoryAllocator/include
)

target_link_libraries(
        VkVixen
        PUBLIC
        GPUOpen::VulkanMemoryAllocator
)
