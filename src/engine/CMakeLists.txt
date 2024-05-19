project("Vixen Engine")

find_package(GLFW3 3.3 REQUIRED)
find_package(assimp REQUIRED)
find_package(glm REQUIRED)
pkg_check_modules(SPDLOG REQUIRED IMPORTED_TARGET spdlog)

find_path(stb_image_INCLUDE_DIRS NAMES stb_image.h PATH_SUFFIXES include include/stb stb REQUIRED)

add_definitions(-DGLFW_INCLUDE_NONE)

add_library(
        Vixen
        STATIC
        src/Window.cpp
        src/Camera.cpp
        src/CommandBuffer.h
        src/CommandPoolUsage.h
        src/IndexFormat.h
        src/PrimitiveTopology.h
        src/VertexAttribute.h
        src/AttachmentInfo.h
        src/LoadAction.h
        src/StoreAction.h
        src/Rectangle.h
        src/BufferUsage.h
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
        ${stb_image_INCLUDE_DIRS}
)

if (ENABLE_VULKAN)
    add_subdirectory(src/platform/vk)
endif ()
