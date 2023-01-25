project("Vixen Engine Vk")

find_package(Vulkan REQUIRED)

set(VK_HEADERS VkWindow.h Macro.h GraphicsCard.h Instance.h ../buffer/vk/VkBuffer.h)
set(VK_SOURCES VkWindow.cpp Instance.cpp Device.cpp Device.h ../buffer/vk/VkBuffer.cpp)

add_library(Vk SHARED ${VK_SOURCES} ${VK_HEADERS})
target_link_libraries(Vk PUBLIC engine Vulkan::Vulkan)
target_compile_definitions(Vk PRIVATE VMA_IMPLEMENTATION)
