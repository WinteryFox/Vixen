project("Vixen Engine Gl")

find_package(OpenGL REQUIRED)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)
find_package(spirv_cross_reflect CONFIG REQUIRED)
find_package(spirv_cross_cpp CONFIG REQUIRED)
pkg_check_modules(GLEW REQUIRED IMPORTED_TARGET glew)

set(GL_HEADERS Window.h ShaderModule.h ShaderProgram.h)
set(GL_SOURCES Window.cpp ShaderModule.cpp ShaderProgram.cpp Buffer.cpp Buffer.h)

add_library(Gl SHARED ${GL_SOURCES} ${GL_HEADERS})
target_link_libraries(Gl engine OpenGL::GL PkgConfig::GLEW spirv-cross-core spirv-cross-glsl spirv-cross-reflect spirv-cross-cpp)
