#pragma once

#include "MeshCache.h"
#include "../gl/Buffer.h"

namespace Vixen::Engine::Cache {
    class OpenGLMeshCache : MeshCache<Gl::Buffer> {
    public:
        explicit OpenGLMeshCache();

    protected:
        Gl::Buffer load(const std::string &path, AllocationUsage allocationUsage, BufferUsage bufferUsage) override;
    };
}
