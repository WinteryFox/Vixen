project("Vixen Engine")

pkg_check_modules(GLM REQUIRED IMPORTED_TARGET glm)
pkg_check_modules(GLFW REQUIRED IMPORTED_TARGET glfw3)
pkg_check_modules(ASSIMP REQUIRED IMPORTED_TARGET assimp)
pkg_check_modules(SPDLOG REQUIRED IMPORTED_TARGET spdlog)
pkg_check_modules(SHADERC REQUIRED IMPORTED_TARGET shaderc)

option(ENABLE_VULKAN "Enable or disable Vulkan support" ON)
option(ENABLE_OPENGL "Enable or disable OpenGL support" ON)

set(HEADERS Window.h ShaderModule.h ShaderProgram.h Monitor.h Buffer.h Mesh.h)
set(SOURCES Window.cpp ShaderModule.cpp)

link_libraries(
        PkgConfig::GLM
        PkgConfig::GLFW
        PkgConfig::ASSIMP
        PkgConfig::SPDLOG
        PkgConfig::SHADERC
)
add_library(engine STATIC ${SOURCES} ${HEADERS})
add_definitions(-DGLFW_INCLUDE_NONE)

if (ENABLE_VULKAN)
    add_subdirectory(vk)
endif ()

if (ENABLE_OPENGL)
    add_subdirectory(gl)
endif ()
