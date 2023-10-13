project("Vixen Engine")

pkg_check_modules(GLM REQUIRED IMPORTED_TARGET glm)
pkg_check_modules(GLFW REQUIRED IMPORTED_TARGET glfw3)
pkg_check_modules(ASSIMP REQUIRED IMPORTED_TARGET assimp)
pkg_check_modules(SPDLOG REQUIRED IMPORTED_TARGET spdlog)

set(HEADERS Window.h ShaderModule.h ShaderProgram.h Monitor.h buffer/Buffer.h Mesh.h Util.h Renderer.h buffer/WritableBuffer.h buffer/ReadableBuffer.h buffer/ReadableWritableBuffer.h Vixen.h Framebuffer.h RenderPass.h Material.h Entity.h)
set(SOURCES Window.cpp buffer/Buffer.cpp buffer/ReadableBuffer.cpp buffer/WritableBuffer.cpp buffer/ReadableWritableBuffer.cpp Blending.h)

link_libraries(
        PkgConfig::GLM
        PkgConfig::GLFW
        PkgConfig::ASSIMP
        PkgConfig::SPDLOG
)
add_library(Vixen SHARED ${SOURCES} ${HEADERS})

if (ENABLE_VULKAN)
    add_subdirectory(vk)
endif ()

if (ENABLE_OPENGL)
    add_subdirectory(gl)
endif ()
