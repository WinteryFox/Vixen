project("Vixen Engine Vk")

pkg_check_modules(Vulkan REQUIRED IMPORTED_TARGET Vulkan>=1.3)

set(VK_HEADERS VkWindow.h Macro.h GraphicsCard.h Instance.h)
set(VK_SOURCES VkWindow.cpp Instance.cpp Device.cpp Device.h)

add_library(Vk SHARED ${VK_SOURCES} ${VK_HEADERS})
target_link_libraries(Vk PUBLIC engine PkgConfig::Vulkan)
