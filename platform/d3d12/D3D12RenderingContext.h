#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>

#include "core/RenderingContext.h"

namespace Vixen {
    class D3D12RenderingContext final : public RenderingContext {
        ID3D12Debug* debugController;

        IDXGIFactory6* factory;

    public:
        D3D12RenderingContext();

        ~D3D12RenderingContext() override;
    };
}
