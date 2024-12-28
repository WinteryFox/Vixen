#include "D3D12RenderingDevice.h"

#include "D3D12RenderingContext.h"

namespace Vixen {
    D3D12RenderingDevice::D3D12RenderingDevice(
        const std::shared_ptr<D3D12RenderingContext> &renderingContext
    ) : RenderingDevice(dynamic_pointer_cast<RenderingContext>(renderingContext)) {
    }

    D3D12RenderingDevice::~D3D12RenderingDevice() {
    }

    Buffer D3D12RenderingDevice::createBuffer(BufferUsage usage, uint32_t count, uint32_t stride) {
    }

    Image D3D12RenderingDevice::createImage(const ImageFormat &format, const ImageView &view) {
    }
}
