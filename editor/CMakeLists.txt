project("Vixen Editor")

set(CMAKE_CXX_STANDARD_REQUIRED ON)  # Qt requires that this is enforced to at least C++ 17
set(AUTOUIC ON)  # Automatically compiles Qt UI files

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
qt_standard_project_setup()

add_executable(qttest qt/main.cpp qt/mainwindow.ui)
target_link_libraries(qttest engine)
target_include_directories(qttest PUBLIC ../engine)
target_link_libraries(qttest Qt6::Core Qt6::Widgets)

set_target_properties(qttest PROPERTIES
        WIN32_EXECUTABLE ON
        MACOSX_BUNDLE ON
)

if (ENABLE_VULKAN)
    add_executable(vktest vk/main.cpp)
    target_link_libraries(vktest engine)
    target_include_directories(vktest PUBLIC ../engine)
    target_link_libraries(vktest Vk)
endif ()

if (ENABLE_OPENGL)
    add_executable(gltest gl/main.cpp)
    target_link_libraries(gltest engine)
    target_include_directories(gltest PUBLIC ../engine)
    target_link_libraries(gltest Gl)
endif ()
