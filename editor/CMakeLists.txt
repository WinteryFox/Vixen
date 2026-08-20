add_executable(
        editor
        main.cpp
)
vixen_configure_target(editor)
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

if (ENABLE_OPENGL)
    target_link_libraries(
            editor
            PUBLIC
            OpenGLVixen
    )
endif ()

set_target_properties(
        editor
        PROPERTIES
        WIN32_EXECUTABLE $<CONFIG:Release>
        MACOSX_BUNDLE $<CONFIG:Release>
)
