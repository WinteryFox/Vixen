set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}")

find_package(D3D12 REQUIRED)

add_library(
        VixenD3D12
        STATIC
        D3D12RenderingContext.cpp
        D3D12RenderingDevice.cpp
)
target_sources(
        VixenD3D12
        PUBLIC
        FILE_SET public_headers
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        D3D12RenderingContext.h
        D3D12RenderingDevice.h
)
vixen_configure_target(VixenD3D12)
target_link_libraries(
        VixenD3D12
        PUBLIC
        VixenCore
        ${D3D12_LIBRARIES}
)
target_include_directories(
        VixenD3D12
        PUBLIC
        ${D3D12_INCLUDE_DIRS}
)
