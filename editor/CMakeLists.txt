project("Vixen Editor")

if (ENABLE_EDITOR)
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
    qt_standard_project_setup()

    add_executable(editor editor/main.cpp editor/mainwindow.ui editor/widgets/GlViewport.cpp editor/widgets/GlViewport.h)
    target_link_libraries(editor engine)
    target_include_directories(editor PUBLIC ../engine)
    target_link_libraries(editor Qt6::Core Qt6::Widgets)

    set_target_properties(editor PROPERTIES WIN32_EXECUTABLE ON MACOSX_BUNDLE ON AUTOUIC ON)
endif ()

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
