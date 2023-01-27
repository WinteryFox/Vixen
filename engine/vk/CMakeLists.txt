project("Vixen Engine Vk")

find_package(Vulkan REQUIRED)

include(FetchContent)
FetchContent_Declare(
        vma
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
        GIT_TAG 29d492b60c84ca784ea0943efc7d2e6e0f3bdaac
)
FetchContent_Declare(
        volk
        GIT_REPOSITORY https://github.com/zeux/volk
        GIT_TAG 4b829a001b475fa005e29357239695946aaf7a99
)
FetchContent_MakeAvailable(VMA volk)

add_library(VkVixen SHARED VkVixen.cpp VkWindow.cpp Instance.cpp Device.cpp Device.h ../buffer/vk/VkBuffer.cpp Allocator.cpp)
target_link_libraries(VkVixen PUBLIC Vixen volk VulkanMemoryAllocator)
