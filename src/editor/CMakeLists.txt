project("Vixen Editor")

add_executable(editor src/main.cpp)
target_link_libraries(editor PRIVATE Vixen)
target_include_directories(
        editor
        PUBLIC
        ${CMAKE_SOURCE_DIR}/src/engine
)

if (ENABLE_VULKAN)
    target_link_libraries(editor PRIVATE VkVixen)
endif ()

if (CMAKE_BUILD_TYPE MATCHES "Release")
    set_target_properties(editor PROPERTIES WIN32_EXECUTABLE ON MACOSX_BUNDLE ON)
endif ()
