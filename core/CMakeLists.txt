find_package(GLFW3 3.3 REQUIRED)
find_package(assimp REQUIRED)
find_package(glm REQUIRED)
pkg_check_modules(SPDLOG REQUIRED IMPORTED_TARGET spdlog)

find_path(stb_image_INCLUDE_DIRS NAMES stb_image.h PATH_SUFFIXES include include/stb stb REQUIRED)

add_definitions(-DGLFW_INCLUDE_NONE)
add_definitions(-DIMGUI_IMPL_VULKAN_USE_VOLK)

set(
        IMGUI_SOURCES
        ${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui.cpp
        ${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui_demo.cpp
        ${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui_draw.cpp
        ${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui_tables.cpp
        ${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui_widgets.cpp
        ${CMAKE_SOURCE_DIR}/thirdparty/imgui/misc/cpp/imgui_stdlib.cpp
        ${CMAKE_SOURCE_DIR}/thirdparty/imgui/backends/imgui_impl_glfw.cpp
        ${CMAKE_SOURCE_DIR}/thirdparty/imgui/backends/imgui_impl_vulkan.cpp
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
        Samples.h
        shader/ShaderResources.h
        Application.cpp
        Application.h
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
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/thirdparty/imgui
        ${stb_image_INCLUDE_DIRS}
)
