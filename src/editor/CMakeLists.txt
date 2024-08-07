project("Vixen Editor")

set(
        IMGUI_SOURCE_FILES
        vendor/imgui/imgui.cpp
        vendor/imgui/imgui_draw.cpp
        vendor/imgui/imgui_tables.cpp
        vendor/imgui/imgui_widgets.cpp
        vendor/imgui/misc/cpp/imgui_stdlib.cpp
)

add_executable(editor
        src/main.cpp
        ${IMGUI_SOURCE_FILES}
)
target_link_libraries(editor PRIVATE Vixen)
target_include_directories(
        editor
        PUBLIC
        ${CMAKE_SOURCE_DIR}/src/engine
        vendor
)

if (ENABLE_VULKAN)
    target_link_libraries(editor PRIVATE VkVixen)
endif ()

if (CMAKE_BUILD_TYPE MATCHES "Release")
    set_target_properties(editor PROPERTIES WIN32_EXECUTABLE ON MACOSX_BUNDLE ON)
endif ()
