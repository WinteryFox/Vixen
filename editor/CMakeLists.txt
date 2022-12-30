project("Vixen Editor")

add_executable(editor main.cpp)
target_link_libraries(editor engine)
target_include_directories(editor PUBLIC ../engine)
if (ENABLE_VULKAN)
    target_link_libraries(editor Vk)
endif ()

if (ENABLE_OPENGL)
    target_link_libraries(editor Gl)
endif ()
