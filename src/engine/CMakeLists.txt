project("Vixen Engine")

find_package(GLFW3 3.3 REQUIRED)
pkg_check_modules(GLM REQUIRED IMPORTED_TARGET glm)
pkg_check_modules(ASSIMP REQUIRED IMPORTED_TARGET assimp)
pkg_check_modules(SPDLOG REQUIRED IMPORTED_TARGET spdlog)

add_definitions(-DGLFW_INCLUDE_NONE)

set(
        SOURCES
        Window.cpp
        Buffer.cpp
        Camera.cpp
        CommandBuffer.h
        CommandPool.h
)

add_library(Vixen STATIC ${SOURCES})
target_link_libraries(
        Vixen
        PUBLIC
        glfw
        PkgConfig::GLM
        PkgConfig::ASSIMP
        PkgConfig::SPDLOG
)

if (ENABLE_VULKAN)
    add_subdirectory(vk)
endif ()

if (ENABLE_OPENGL)
    add_subdirectory(gl)
endif ()
