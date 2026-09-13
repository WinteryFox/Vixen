#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace Vixen {
    enum class DescriptorErrorCode {
        InvalidArgument,
        InvalidPoolState,
        ExpiredSet,
        IncompatibleLayout,
        SetNotFound,
        BindingNotFound,
        DescriptorTypeMismatch,
        ArrayRangeOutOfBounds,
        ResourceTypeMismatch,
        ResourceUsageMismatch,
        ResourceRangeOutOfBounds,
        InvalidImageLayout,
        MisalignedOffset,
        UnsupportedUsage,
        PoolExhausted,
        OutOfHostMemory,
        OutOfDeviceMemory,
        NativeOperationFailed
    };

    struct NativeDescriptorError {
        std::string backend;
        std::string operation;
        int64_t code;
        std::string name;
    };

    struct DescriptorError {
        DescriptorErrorCode code;
        std::string message;

        std::optional<uint32_t> set = std::nullopt;
        std::optional<uint32_t> binding = std::nullopt;
        std::optional<uint32_t> arrayElement = std::nullopt;
        std::optional<NativeDescriptorError> nativeError = std::nullopt;
        std::vector<std::string> details{};
    };
}
