project("Vixen Editor")

add_executable(editor main.cpp)
target_link_libraries(editor engine)
target_include_directories(editor PUBLIC ../engine)
