project("Vixen Engine")

find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)
find_package(spirv_cross_reflect CONFIG REQUIRED)
find_package(spirv_cross_cpp CONFIG REQUIRED)

pkg_check_modules(GLM REQUIRED IMPORTED_TARGET glm)
pkg_check_modules(GLFW REQUIRED IMPORTED_TARGET glfw3)
pkg_check_modules(ASSIMP REQUIRED IMPORTED_TARGET assimp)
pkg_check_modules(SPDLOG REQUIRED IMPORTED_TARGET spdlog)
pkg_check_modules(GLSLANG REQUIRED IMPORTED_TARGET glslang)

option(ENABLE_VULKAN "Enable or disable Vulkan support" ON)
option(ENABLE_OPENGL "Enable or disable OpenGL support" ON)

set(HEADERS Window.h ShaderModule.h ShaderProgram.h Monitor.h Buffer.h Mesh.h cache/BufferCache.h cache/MeshCache.h)
set(SOURCES Window.cpp ShaderModule.cpp cache/BufferCache.cpp cache/MeshCache.cpp)

link_libraries(
        PkgConfig::GLM
        PkgConfig::GLFW
        PkgConfig::ASSIMP
        PkgConfig::SPDLOG
        PkgConfig::GLSLANG
        spirv-cross-core
        spirv-cross-glsl
        spirv-cross-reflect
        spirv-cross-cpp
)
add_library(engine STATIC ${SOURCES} ${HEADERS})
add_definitions(-DGLFW_INCLUDE_NONE)

if (ENABLE_VULKAN)
    add_subdirectory(vk)
endif ()

if (ENABLE_OPENGL)
    add_subdirectory(gl)
endif ()
