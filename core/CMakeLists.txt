find_package(GLFW3 3.3 REQUIRED)
find_package(assimp REQUIRED)
find_package(glm REQUIRED)
pkg_check_modules(SPDLOG REQUIRED IMPORTED_TARGET spdlog)

find_path(stb_image_INCLUDE_DIRS NAMES stb_image.h PATH_SUFFIXES include include/stb stb REQUIRED)

add_compile_definitions(GLFW_INCLUDE_NONE)

add_library(
        imgui
        STATIC
        ${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui.cpp
        ${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui_demo.cpp
        ${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui_draw.cpp
        ${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui_tables.cpp
        ${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui_widgets.cpp
        ${CMAKE_SOURCE_DIR}/thirdparty/imgui/misc/cpp/imgui_stdlib.cpp
        ${CMAKE_SOURCE_DIR}/thirdparty/imgui/backends/imgui_impl_glfw.cpp
        ${CMAKE_SOURCE_DIR}/thirdparty/imgui/backends/imgui_impl_vulkan.cpp
)
target_include_directories(
        imgui
        PUBLIC
        ${CMAKE_SOURCE_DIR}/thirdparty/imgui
)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_VULKAN_USE_VOLK)

add_library(
        Vixen
        STATIC
        ${IMGUI_SOURCES}
        DisplayServer.cpp
        Camera.cpp
        CommandBuffer.h
        CommandPoolUsage.h
        IndexFormat.h
        PrimitiveTopology.h
        VertexAttribute.h
        AttachmentInfo.h
        LoadAction.h
        StoreAction.h
        Rectangle.h
        buffer/BufferUsage.h
        error/ShaderLinkFailedError.h
        shader/ShaderResources.h
        Application.cpp
        Application.h
        RenderingDevice.h
        error/macros.h
        error/CantCreateError.h
        error/Macros.cpp
        buffer/Buffer.h
        MemoryAllocationType.h
        image/ImageView.h
        DataFormat.h
        image/ImageSwizzle.h
        image/ImageLayout.h
        image/ImageAspect.h
        image/ImageSubresource.h
        image/ImageFormat.h
        image/ImageSamples.h
        image/ImageType.h
        image/Image.cpp
        image/Image.h
        image/ImageUsage.h
        image/Sampler.cpp
        image/Sampler.h
        image/SamplerState.h
        image/SamplerFilter.h
        image/SamplerRepeatMode.h
        image/CompareOperator.h
        image/SamplerBorderColor.h
        Bitmask.h
        buffer/Buffer.cpp
)
target_link_libraries(
        Vixen
        PUBLIC
        glfw
        assimp::assimp
        glm::glm
        PkgConfig::SPDLOG
        imgui
)
target_include_directories(
        Vixen
        PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/thirdparty/imgui
        ${stb_image_INCLUDE_DIRS}
)
