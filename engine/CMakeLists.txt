project("Vixen Engine")

pkg_check_modules(GLM REQUIRED IMPORTED_TARGET glm)
pkg_check_modules(GLFW REQUIRED IMPORTED_TARGET glfw3)
pkg_check_modules(ASSIMP REQUIRED IMPORTED_TARGET assimp)
pkg_check_modules(SPDLOG REQUIRED IMPORTED_TARGET spdlog)

find_path(FreeImage_INCLUDE_DIR FreeImage.h HINTS ${FreeImage_ROOT})
find_library(FreeImage_LIBRARY NAMES FreeImage HINTS ${FreeImage_ROOT})
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FreeImage DEFAULT_MSG FreeImage_INCLUDE_DIR FreeImage_LIBRARY)

set(HEADERS Window.h ShaderModule.h ShaderProgram.h Monitor.h Buffer.h Mesh.h Util.h Renderer.h Vixen.h Framebuffer.h RenderPass.h Material.h Entity.h)
set(SOURCES Window.cpp Buffer.cpp Blending.h)

link_libraries(
        PkgConfig::GLM
        PkgConfig::GLFW
        PkgConfig::ASSIMP
        PkgConfig::SPDLOG
        FreeImage
)
add_library(Vixen SHARED ${SOURCES} ${HEADERS})

if (ENABLE_VULKAN)
    add_subdirectory(vk)
endif ()

if (ENABLE_OPENGL)
    add_subdirectory(gl)
endif ()
