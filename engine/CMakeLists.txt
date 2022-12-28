project("Vixen Engine")

pkg_check_modules(GLM REQUIRED IMPORTED_TARGET glm)
pkg_check_modules(GLFW REQUIRED IMPORTED_TARGET glfw3)
pkg_check_modules(ASSIMP REQUIRED IMPORTED_TARGET assimp)
pkg_check_modules(SPDLOG REQUIRED IMPORTED_TARGET spdlog)

option(ENABLE_VULKAN "Enable or disable Vulkan support" ON)
option(ENABLE_OPENGL "Enable or disable OpenGL support" ON)

if (ENABLE_VULKAN)
    message(STATUS "Vulkan support is enabled")
    add_definitions(-DVK_ENABLED=1)

    set(VK_HEADERS vk/Window.h)
    set(VK_SOURCES vk/Window.cpp)
else ()
    message(STATUS "Vulkan support is disabled")
endif ()

if (ENABLE_OPENGL)
    message(STATUS "OpenGL support is enabled")
    add_definitions(-DGL_ENABLED=1)

    set(GL_HEADERS gl/Window.h)
    set(GL_SOURCES gl/Window.cpp)
else ()
    message(STATUS "OpenGL support is disabled")
endif ()

set(HEADERS Window.hpp)
set(SOURCES Window.cpp)

add_library(engine ${SOURCES} ${VK_SOURCES} ${GL_SOURCES})
target_link_libraries(
        engine
        PkgConfig::GLM
        PkgConfig::GLFW
        PkgConfig::ASSIMP
        PkgConfig::SPDLOG
)

if (ENABLE_VULKAN)
    find_package(Vulkan REQUIRED)
    target_link_libraries(engine Vulkan::Vulkan)
endif ()

if (ENABLE_OPENGL)
    find_package(OpenGL REQUIRED)
    pkg_check_modules(GLEW REQUIRED IMPORTED_TARGET glew)
    target_link_libraries(engine OpenGL::GL PkgConfig::GLEW)
endif ()
