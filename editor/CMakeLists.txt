add_executable(
        VixenEditor
        main.cpp
)
vixen_configure_target(VixenEditor)
target_link_libraries(
        VixenEditor
        PUBLIC
        VixenCore
)

if (ENABLE_VULKAN)
    target_link_libraries(
            VixenEditor
            PUBLIC
            VixenVulkan
    )
endif ()

if (ENABLE_D3D12)
    target_link_libraries(
            VixenEditor
            PUBLIC
            VixenD3D12
    )
endif ()

if (ENABLE_OPENGL)
    target_link_libraries(
            VixenEditor
            PUBLIC
            VixenOpenGL
    )
endif ()

set_target_properties(
        VixenEditor
        PROPERTIES
        WIN32_EXECUTABLE $<CONFIG:Release>
        MACOSX_BUNDLE $<CONFIG:Release>
)
