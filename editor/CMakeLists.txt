project("Vixen Editor")

add_executable(
        editor
        main.cpp
)
target_include_directories(
        editor
        PUBLIC
        ${CMAKE_SOURCE_DIR}/src/engine
)
target_link_libraries(
        editor
        PUBLIC
        Vixen
)

if (ENABLE_VULKAN)
    target_link_libraries(
            editor
            PUBLIC
            VkVixen
    )
endif ()

if (ENABLE_D3D12)
    target_link_libraries(
            editor
            PUBLIC
            D3D12Vixen
    )
endif ()

if (CMAKE_BUILD_TYPE MATCHES "Release")
    set_target_properties(editor PROPERTIES WIN32_EXECUTABLE ON MACOSX_BUNDLE ON)
endif ()
