project("Vixen Engine")

pkg_check_modules(GLM REQUIRED IMPORTED_TARGET GLM)
pkg_check_modules(GLFW REQUIRED IMPORTED_TARGET GLFW3)
pkg_check_modules(ASSIMP REQUIRED IMPORTED_TARGET ASSIMP)
pkg_check_modules(SPDLOG REQUIRED IMPORTED_TARGET SPDLOG)

option(ENABLE_VULKAN "Enable or disable Vulkan support" ON)
option(ENABLE_OPENGL "Enable or disable OpenGL support" ON)

if (ENABLE_VULKAN)
    message(STATUS "Vulkan support is enabled")
    add_definitions(-DVK_ENABLED=1)
else ()
    message(STATUS "Vulkan support is disabled")
endif ()

if (ENABLE_OPENGL)
    message(STATUS "OpenGL support is enabled")
    add_definitions(-DGL_ENABLED=1)
    set(GL_HEADERS GlWindow.h)
    set(GL_SOURCES GlWindow.cpp)
else ()
    message(STATUS "OpenGL support is disabled")
endif ()

set(HEADERS Window.hpp)
set(SOURCES Window.cpp)

add_library(engine ${SOURCES} ${VULKAN_SOURCES} ${GL_SOURCES})
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
    pkg_check_modules(GLEW REQUIRED IMPORTED_TARGET GLEW)
    target_link_libraries(engine OpenGL::GL PkgConfig::GLEW)
endif ()
