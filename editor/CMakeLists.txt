project("Vixen Editor")

if (ENABLE_EDITOR)
    find_package(Qt6 REQUIRED COMPONENTS Core Qml Widgets)
    qt_standard_project_setup()

    add_executable(editor main.cpp mainwindow.ui widgets/GlViewport.cpp widgets/GlViewport.h)
    target_include_directories(editor PUBLIC ../engine)
    target_link_libraries(editor PRIVATE engine Qt6::Core Qt6::Qml Qt6::Widgets)
    qt_add_qml_module(editor URI resources VERSION 1.0 QML_FILES main.qml)

    if (ENABLE_OPENGL)
        find_package(Qt6 REQUIRED COMPONENTS OpenGLWidgets)
        target_link_libraries(editor PRIVATE Qt6::OpenGLWidgets)
    endif ()

    # TODO
    #[[if (ENABLE_VULKAN)
        find_package(Qt6 REQUIRED COMPONENTS VulkanWidgets)
        target_link_libraries(editor Qt6::VulkanWidgets)
    endif ()]]

    set_target_properties(editor PROPERTIES WIN32_EXECUTABLE ON MACOSX_BUNDLE ON AUTOUIC ON)
endif ()

if (ENABLE_VULKAN)
    add_executable(vktest test/vk/main.cpp)
    target_link_libraries(vktest engine)
    target_include_directories(vktest PUBLIC ../engine)
    target_link_libraries(vktest test/vk)
endif ()

if (ENABLE_OPENGL)
    add_executable(gltest test/gl/main.cpp)
    target_link_libraries(gltest engine)
    target_include_directories(gltest PUBLIC ../engine)
    target_link_libraries(gltest test/gl)
endif ()
