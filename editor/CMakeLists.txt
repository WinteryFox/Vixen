project("Vixen Editor")

find_package(Qt6 6.3 REQUIRED COMPONENTS Core Qml Widgets Quick)
qt_standard_project_setup()
qt_policy(SET QTP0001 NEW)

add_executable(editor main.cpp)
target_include_directories(editor PUBLIC ../engine)
target_link_libraries(editor PRIVATE Vixen Qt6::Core Qt6::Qml Qt6::Quick Qt6::Widgets)

qt_add_qml_module(editor URI editor VERSION 1.0 QML_FILES ui/main.qml ui/Viewport.qml ui/Dock.qml)

if (ENABLE_OPENGL)
    target_link_libraries(editor PRIVATE GlVixen)
endif ()

if (ENABLE_VULKAN)
    target_link_libraries(editor PRIVATE VkVixen)
endif ()

set_target_properties(editor PROPERTIES WIN32_EXECUTABLE ON MACOSX_BUNDLE ON AUTOUIC ON)
