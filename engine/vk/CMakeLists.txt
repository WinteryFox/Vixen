project("Vixen Engine Vk")

find_package(Vulkan REQUIRED)

set(VK_HEADERS VkWindow.h Macro.h GraphicsCard.h Instance.h)
set(VK_SOURCES VkWindow.cpp Instance.cpp Device.cpp Device.h)

add_library(Vk STATIC ${VK_SOURCES} ${VK_HEADERS})
target_link_libraries(Vk PUBLIC engine Vulkan::Vulkan)
