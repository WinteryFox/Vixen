#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace Vixen {
    enum class ResourceCreationErrorCode {
        InvalidDescription,
        UnsupportedFormat,
        UnsupportedUsage,
        UnsupportedSampleCount,
        ExceedsDeviceLimits,
        OutOfDeviceMemory,
        OutOfHostMemory,
        NativeObjectCreationFailed,
        NativeViewCreationFailed,
        CompatibilityError,
        ExtentExceedsDeviceLimits,
        MipCountExceedsDeviceLimits,
        LayerCountExceedsDeviceLimits
    };

    struct NativeResourceCreationError {
        std::string backend;

        std::string operation;

        int64_t code;

        std::string name;
    };

    struct ResourceCreationLimitViolation {
        std::string limit;

        uint64_t requested;

        uint64_t supported;
    };

    struct ResourceCreationError {
        ResourceCreationErrorCode code;

        std::string message;

        std::optional<NativeResourceCreationError> nativeError{};

        std::optional<ResourceCreationLimitViolation> limitViolation{};

        std::vector<std::string> details{};
    };
}
