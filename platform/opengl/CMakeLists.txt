add_library(
        VixenOpenGL
        STATIC
        OpenGLRenderingDevice.cpp
        OpenGLRenderingContext.cpp
)
target_sources(
        VixenOpenGL
        PUBLIC
        FILE_SET public_headers
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        OpenGLRenderingContext.h
        OpenGLRenderingDevice.h
)
vixen_configure_target(VixenOpenGL)
target_link_libraries(
        VixenOpenGL
        PUBLIC
        VixenCore
)
