#pragma once

#include <Volk/volk.h>
#include <vma/vk_mem_alloc.h>
#include "../Vixen.h"
#include "VkWindow.h"
#include "Instance.h"
#include "Device.h"

namespace Vixen::Engine {
    class VkVixen : public Vixen {
    public:
        const std::vector<const char *> deviceExtensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        VkWindow window;

        Instance instance;

        Device device;

        VmaAllocator allocator;

        explicit VkVixen(const std::string &appTitle);

        VkVixen(const VkVixen &) = delete;

        VkVixen &operator=(const VkVixen &) = delete;

        ~VkVixen();
    };
}
