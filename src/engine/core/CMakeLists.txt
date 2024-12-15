project("Core")

find_package(GLFW3 3.3 REQUIRED)
find_package(assimp REQUIRED)
find_package(glm REQUIRED)
pkg_check_modules(SPDLOG REQUIRED IMPORTED_TARGET spdlog)

find_path(stb_image_INCLUDE_DIRS NAMES stb_image.h PATH_SUFFIXES include include/stb stb REQUIRED)

add_definitions(-DGLFW_INCLUDE_NONE)
add_definitions(-DIMGUI_IMPL_VULKAN_USE_VOLK)

set(
        IMGUI_SOURCES
        vendor/imgui/imgui.cpp
        vendor/imgui/imgui_demo.cpp
        vendor/imgui/imgui_draw.cpp
        vendor/imgui/imgui_tables.cpp
        vendor/imgui/imgui_widgets.cpp
        vendor/imgui/misc/cpp/imgui_stdlib.cpp
        vendor/imgui/backends/imgui_impl_glfw.cpp
        vendor/imgui/backends/imgui_impl_vulkan.cpp
)

add_library(
        Vixen
        STATIC
        ${IMGUI_SOURCES}
        Window.cpp
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
        BufferUsage.h
        exception/ShaderLinkFailedException.h
        Application.h
        Samples.h
        shader/ShaderResources.h
)
target_link_libraries(
        Vixen
        PUBLIC
        glfw
        assimp::assimp
        glm::glm
        PkgConfig::SPDLOG
)
target_include_directories(
        Vixen
        PUBLIC
        vendor/imgui
        ${stb_image_INCLUDE_DIRS}
)
