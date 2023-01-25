project("Vixen Engine")

find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)
find_package(spirv_cross_reflect CONFIG REQUIRED)
find_package(spirv_cross_cpp CONFIG REQUIRED)
find_package(glslang REQUIRED)

pkg_check_modules(GLM REQUIRED IMPORTED_TARGET glm)
pkg_check_modules(GLFW REQUIRED IMPORTED_TARGET glfw3)
pkg_check_modules(ASSIMP REQUIRED IMPORTED_TARGET assimp)
pkg_check_modules(SPDLOG REQUIRED IMPORTED_TARGET spdlog)

option(ENABLE_VULKAN "Enable or disable Vulkan support" ON)
option(ENABLE_OPENGL "Enable or disable OpenGL support" ON)

set(HEADERS Window.h ShaderModule.h ShaderProgram.h Monitor.h buffer/Buffer.h Mesh.h Util.h Renderer.h buffer/WritableBuffer.h buffer/ReadableBuffer.h buffer/ReadableWritableBuffer.h Vixen.h)
set(SOURCES Window.cpp ShaderModule.cpp buffer/Buffer.cpp buffer/ReadableBuffer.cpp buffer/WritableBuffer.cpp buffer/ReadableWritableBuffer.cpp)

link_libraries(
        PkgConfig::GLM
        PkgConfig::GLFW
        PkgConfig::ASSIMP
        PkgConfig::SPDLOG
#        glslang::OSDependent
        glslang::glslang
#        glslang::MachineIndependent
#        glslang::GenericCodeGen
#        glslang::OGLCompiler
#        glslang::glslangValidator
#        glslang::spirv-remap
        glslang::glslang-default-resource-limits
#        glslang::SPVRemapper
        glslang::SPIRV
#        glslang::HLSL
        spirv-cross-core
        spirv-cross-glsl
        spirv-cross-reflect
        spirv-cross-cpp
)
add_library(engine SHARED ${SOURCES} ${HEADERS})
add_definitions(-DGLFW_INCLUDE_NONE)

if (ENABLE_VULKAN)
    add_subdirectory(vk)
endif ()

if (ENABLE_OPENGL)
    add_subdirectory(gl)
endif ()
