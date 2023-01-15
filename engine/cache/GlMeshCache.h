#pragma once

#include "MeshCache.h"
#include "../gl/GlBuffer.h"

namespace Vixen::Engine::Cache {
    class GlMeshCache : MeshCache<Gl::GlBuffer> {
    public:
        explicit GlMeshCache();

    protected:
        Gl::GlBuffer load(const std::string &path, AllocationUsage allocationUsage, BufferUsage bufferUsage) override;
    };
}
