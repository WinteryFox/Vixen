project("Vixen Engine")

add_subdirectory(core)

if (ENABLE_VULKAN)
    add_subdirectory(platform/vulkan)
endif ()
