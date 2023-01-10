project("Vixen Engine Gl")

find_package(OpenGL REQUIRED)
pkg_check_modules(GLEW REQUIRED IMPORTED_TARGET glew)

set(GL_HEADERS Window.h ShaderModule.h ShaderProgram.h VertexArrayObject.h ../cache/OpenGLMeshCache.h)
set(GL_SOURCES Window.cpp ShaderModule.cpp ShaderProgram.cpp Buffer.cpp Buffer.h VertexArrayObject.cpp ../cache/OpenGLMeshCache.cpp)

add_library(Gl STATIC ${GL_SOURCES} ${GL_HEADERS})
target_link_libraries(Gl engine OpenGL::GL PkgConfig::GLEW)
