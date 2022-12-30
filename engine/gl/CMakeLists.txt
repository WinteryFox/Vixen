project("Vixen Engine Gl")

find_package(OpenGL REQUIRED)
pkg_check_modules(GLEW REQUIRED IMPORTED_TARGET glew)
pkg_check_modules(SPIRV_CROSS REQUIRED IMPORTED_TARGET spirv-cross-c-shared)

set(GL_HEADERS Window.h ShaderModule.h ShaderProgram.h)
set(GL_SOURCES Window.cpp ShaderModule.cpp ShaderProgram.cpp)

add_library(Gl SHARED ${GL_SOURCES})
target_link_libraries(Gl engine OpenGL::GL PkgConfig::GLEW PkgConfig::SPIRV_CROSS)
