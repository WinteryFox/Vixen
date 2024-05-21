project("Core")

find_package(GLFW3 3.3 REQUIRED)
find_package(assimp REQUIRED)
find_package(glm REQUIRED)
pkg_check_modules(SPDLOG REQUIRED IMPORTED_TARGET spdlog)

find_path(stb_image_INCLUDE_DIRS NAMES stb_image.h PATH_SUFFIXES include include/stb stb REQUIRED)

add_definitions(-DGLFW_INCLUDE_NONE)

add_library(
        Vixen
        STATIC
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
        ${stb_image_INCLUDE_DIRS}
)
