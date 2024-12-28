#pragma once

#include <memory>

#include "core/RenderingDevice.h"

namespace Vixen {
    class D3D12RenderingContext;

    class D3D12RenderingDevice : public RenderingDevice {
    public:
        explicit D3D12RenderingDevice(const std::shared_ptr<D3D12RenderingContext> &renderingContext);

        ~D3D12RenderingDevice() override;

        Buffer createBuffer(Buffer::Usage usage, uint32_t count, uint32_t stride) override;

        Image createImage(const ImageFormat &format, const ImageView &view) override;
    };
}
