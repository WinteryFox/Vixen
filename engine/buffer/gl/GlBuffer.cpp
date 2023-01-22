#include "GlBuffer.h"

namespace Vixen::Engine {
    GlBuffer::GlBuffer(GLbitfield flags, const size_t &size, BufferUsage bufferUsage, AllocationUsage allocationUsage)
            : Buffer(size, bufferUsage, allocationUsage), buffer(), flags(flags) {
        flags |= GL_MAP_COHERENT_BIT |
                 GL_MAP_PERSISTENT_BIT;

        glCreateBuffers(1, &buffer);
        switch (allocationUsage) {
            case AllocationUsage::GPU_ONLY:
            case AllocationUsage::CPU_TO_GPU:
                break;
            case AllocationUsage::CPU_ONLY:
            case AllocationUsage::GPU_TO_CPU:
                flags |= GL_CLIENT_STORAGE_BIT;
                break;
        }

        // TODO
        if (bufferUsage & BufferUsage::VERTEX);
        if (bufferUsage & BufferUsage::INDEX);
        if (bufferUsage & BufferUsage::TRANSFER_DST);
        if (bufferUsage & BufferUsage::TRANSFER_SRC);
        if (bufferUsage & BufferUsage::UNIFORM);

        glNamedBufferStorage(
                buffer,
                static_cast<GLsizeiptr>(size),
                nullptr,
                flags
        );
        dataPointer = map(0, size, flags);
    }

    GlBuffer::GlBuffer(GLuint buffer, GLbitfield flags, const size_t &size, BufferUsage bufferUsage,
                       AllocationUsage allocationUsage)
            : Buffer(size, bufferUsage, allocationUsage), buffer(buffer), flags(flags) {
        dataPointer = map(0, size, flags);
    }

    GlBuffer::~GlBuffer() {
        glUnmapNamedBuffer(buffer);
        glDeleteBuffers(1, &buffer);
    }

    void *GlBuffer::map(std::size_t offset, std::size_t length, GLbitfield mappingFlags) const {
        if (offset > size)
            error("Offset is greater than the total buffer size");
        if (offset + length > size)
            error("The offset plus length is greater than the total buffer size");

        return glMapNamedBufferRange(buffer, static_cast<GLsizeiptr>(offset), static_cast<GLsizeiptr>(length),
                                     mappingFlags);
    }
}
