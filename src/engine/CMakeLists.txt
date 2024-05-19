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
        src/core/Window.cpp
        src/core/Camera.cpp
        src/core/CommandBuffer.h
        src/core/CommandPoolUsage.h
        src/core/IndexFormat.h
        src/core/PrimitiveTopology.h
        src/core/VertexAttribute.h
        src/core/AttachmentInfo.h
        src/core/LoadAction.h
        src/core/StoreAction.h
        src/core/Rectangle.h
        src/core/BufferUsage.h
        src/core/FrameData.h
        src/core/exception/ShaderLinkFailedException.h
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
    add_subdirectory(src/platform/vulkan)
endif ()
