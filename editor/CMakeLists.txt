project("Vixen Editor")

if (ENABLE_EDITOR)
    find_package(Qt6 REQUIRED COMPONENTS Core Qml Widgets Quick)
    qt_standard_project_setup()

    add_executable(editor main.cpp widgets/GlViewport.cpp widgets/GlViewport.h widgets/GlViewportRenderer.cpp widgets/GlViewportRenderer.h)
    target_include_directories(editor PUBLIC ../engine)
    target_link_libraries(editor PRIVATE engine Qt6::Core Qt6::Qml Qt6::Quick Qt6::Widgets)

    qt_add_qml_module(editor URI editor VERSION 1.0 QML_FILES main.qml Viewport.qml)

    if (ENABLE_OPENGL)
        target_link_libraries(editor PRIVATE Gl)
    endif ()

    if (ENABLE_VULKAN)
        target_link_libraries(editor PRIVATE Vk)
    endif ()

    set_target_properties(editor PROPERTIES WIN32_EXECUTABLE ON MACOSX_BUNDLE ON AUTOUIC ON)
endif ()

if (ENABLE_VULKAN)
    add_executable(vktest test/vk/main.cpp)
    target_link_libraries(vktest PUBLIC engine Vk)
    target_include_directories(vktest PUBLIC ../engine)
endif ()

if (ENABLE_OPENGL)
    add_executable(gltest test/gl/main.cpp)
    target_link_libraries(gltest PUBLIC engine Gl)
    target_include_directories(gltest PUBLIC ../engine)
endif ()
