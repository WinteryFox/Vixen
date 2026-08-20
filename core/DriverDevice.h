#pragma once

#include <cstdint>
#include <string>

namespace Vixen {
    enum class DriverDeviceType {
        Other,
        Integrated,
        Discrete,
        Virtual,
        Cpu
    };

    struct DriverDevice {
        std::string name;
        DriverDeviceType type = DriverDeviceType::Other;
        uint64_t deviceLocalMemory = 0;
        bool hasDedicatedComputeQueue = false;
        bool hasDedicatedTransferQueue = false;
    };
}
