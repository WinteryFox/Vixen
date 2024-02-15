project("Vixen Engine")

find_package(GLFW3 3.3 REQUIRED)
find_package(assimp REQUIRED)
find_package(glm REQUIRED)
pkg_check_modules(SPDLOG REQUIRED IMPORTED_TARGET spdlog)

add_definitions(-DGLFW_INCLUDE_NONE)

set(
        SOURCES
        Window.cpp
        Camera.cpp
        CommandBuffer.h
        CommandPool.h
        IndexFormat.h
        PrimitiveTopology.h
        VertexAttribute.h
        AttachmentInfo.h
        LoadAction.h
        StoreAction.h
        Rectangle.h
)

add_library(Vixen STATIC ${SOURCES})
target_link_libraries(
        Vixen
        PUBLIC
        glfw
        assimp::assimp
        glm::glm
        PkgConfig::SPDLOG
)

if (ENABLE_VULKAN)
    add_subdirectory(vk)
endif ()

if (ENABLE_OPENGL)
    add_subdirectory(gl)
endif ()
