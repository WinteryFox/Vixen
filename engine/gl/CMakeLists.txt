project("Vixen Engine Gl")

find_package(OpenGL REQUIRED)
pkg_check_modules(GLEW REQUIRED IMPORTED_TARGET glew)

add_library(GlVixen SHARED GlWindow.cpp GlShaderModule.cpp GlShaderProgram.cpp GlVertexArrayObject.cpp ../buffer/gl/GlBuffer.cpp ../buffer/gl/ReadableGlBuffer.cpp ../buffer/gl/WritableGlBuffer.cpp ../buffer/gl/ReadableWritableGlBuffer.cpp GlVixen.cpp GlRenderer.cpp GlRenderer.h)
target_link_libraries(GlVixen PUBLIC Vixen OpenGL::GL PkgConfig::GLEW)

if (ENABLE_TESTS)
    add_executable(gltest test/main.cpp)
    target_link_libraries(gltest PUBLIC Vixen GlVixen)
    target_include_directories(gltest PUBLIC ../engine)
endif ()
