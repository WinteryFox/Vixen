#pragma once

#include "MeshCache.h"
#include "../gl/GlBuffer.h"

namespace Vixen::Engine::Cache {
    class GlMeshCache : MeshCache<GlBuffer> {
    public:
        explicit GlMeshCache();

    protected:
        GlBuffer load(const std::string &path, AllocationUsage allocationUsage, BufferUsage bufferUsage) override;
    };
}
