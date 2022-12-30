project("Vixen Engine Vk")

find_package(Vulkan REQUIRED)

set(VK_HEADERS Window.h)
set(VK_SOURCES Window.cpp)

add_library(Vk SHARED ${VK_SOURCES} ${VK_HEADERS})
target_link_libraries(Vk engine Vulkan::Vulkan)
