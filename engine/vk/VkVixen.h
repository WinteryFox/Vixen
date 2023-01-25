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
        std::unique_ptr<VkWindow> window;

        std::unique_ptr<Instance> instance;

        std::unique_ptr<Device> device;

        explicit VkVixen(const std::string &appTitle);
    };
}
