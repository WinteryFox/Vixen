#pragma once

#include <functional>

namespace Vixen {
    class RenderingDeviceDriver;

    using DeferredRelease = std::move_only_function<void(RenderingDeviceDriver&)>;
}
