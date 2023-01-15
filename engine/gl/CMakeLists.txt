project("Vixen Engine Gl")

find_package(OpenGL REQUIRED)
pkg_check_modules(GLEW REQUIRED IMPORTED_TARGET glew)

set(GL_HEADERS GlWindow.h GlShaderModule.h GlShaderProgram.h GlVertexArrayObject.h ../cache/GlMeshCache.h)
set(GL_SOURCES GlWindow.cpp GlShaderModule.cpp GlShaderProgram.cpp GlBuffer.cpp GlBuffer.h GlVertexArrayObject.cpp ../cache/GlMeshCache.cpp)

add_library(Gl STATIC ${GL_SOURCES} ${GL_HEADERS})
target_link_libraries(Gl engine OpenGL::GL PkgConfig::GLEW)
