#include "D3D12RenderingContext.h"

#include "core/error/CantCreateError.h"
#include "core/error/Macros.h"

namespace Vixen {
    D3D12RenderingContext::D3D12RenderingContext() {
#ifdef DEBUG_ENABLED
        ASSERT_THROW(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)) == S_OK, CantCreateError,
                     "Failed to get debug interface");
#endif

        ASSERT_THROW(CreateDXGIFactory1(IID_PPV_ARGS(&factory)) == S_OK, CantCreateError,
                     "Failed to create DXGIFactory1");
    }

    D3D12RenderingContext::~D3D12RenderingContext() {

    }
}
