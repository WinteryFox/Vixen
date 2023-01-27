project("Vixen Engine Vk")

find_package(Vulkan REQUIRED)

add_library(VkVixen SHARED VkVixen.cpp VkWindow.cpp Instance.cpp Device.cpp Device.h ../buffer/vk/VkBuffer.cpp Allocator.cpp)
target_link_libraries(VkVixen PUBLIC Vixen Vulkan::Vulkan)
#target_compile_definitions(VkVixen PUBLIC VK_NO_PROTOTYPES=1 VMA_IMPLEMENTATION=1 VMA_STATIC_VULKAN_FUNCTIONS=0 VMA_DYNAMIC_VULKAN_FUNCTIONS=0)
