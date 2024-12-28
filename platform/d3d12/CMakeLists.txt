set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}")

find_package(D3D12 REQUIRED)

add_library(
        D3D12Vixen
        STATIC
        D3D12RenderingContext.cpp
        D3D12RenderingContext.h
        D3D12RenderingDevice.cpp
        D3D12RenderingDevice.h
)
target_link_libraries(
        D3D12Vixen
        PUBLIC
        Vixen
        ${D3D12_LIBRARIES}
)
target_link_directories(
        D3D12Vixen
        PUBLIC
        ${D3D12_INCLUDE_DIRS}
)
