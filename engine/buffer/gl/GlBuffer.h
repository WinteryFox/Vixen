#pragma once

#include <GL/glew.h>
#include <spdlog/spdlog.h>
#include "../Buffer.h"
#include "../../Util.h"

namespace Vixen::Vk {
    class GlBuffer : public Buffer {
        friend class WritableGlBuffer;

        friend class GlVertexArrayObject;

    protected:
        GLuint buffer;

        GLbitfield flags;

        void *dataPointer;

        GlBuffer(GLbitfield flags, const size_t &size, BufferUsage bufferUsage, AllocationUsage allocationUsage);

        GlBuffer(GLuint buffer, GLbitfield flags, const size_t &size, BufferUsage bufferUsage,
                 AllocationUsage allocationUsage);

        ~GlBuffer();

        [[nodiscard]] void *map(std::size_t offset, std::size_t length) const;
    };
}
