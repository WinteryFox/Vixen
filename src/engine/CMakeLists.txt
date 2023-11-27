project("Vixen Engine")

pkg_check_modules(GLM REQUIRED IMPORTED_TARGET glm)
pkg_check_modules(GLFW REQUIRED IMPORTED_TARGET glfw3)
pkg_check_modules(ASSIMP REQUIRED IMPORTED_TARGET assimp)
pkg_check_modules(SPDLOG REQUIRED IMPORTED_TARGET spdlog)

add_definitions(-DGLFW_INCLUDE_NONE)

set(
        SOURCES
        Window.cpp
        Buffer.cpp
        Camera.cpp
)

link_libraries(
        PkgConfig::GLM
        PkgConfig::GLFW
        PkgConfig::ASSIMP
        PkgConfig::SPDLOG
)
add_library(Vixen STATIC ${SOURCES})

if (ENABLE_VULKAN)
    add_subdirectory(vk)
endif ()

if (ENABLE_OPENGL)
    add_subdirectory(gl)
endif ()
