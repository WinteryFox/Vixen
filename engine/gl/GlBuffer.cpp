#include "GlBuffer.h"

namespace Vixen::Gl {
    // TODO: Synchronization
    GlBuffer::GlBuffer(Usage bufferUsage, const size_t &size)
            : Buffer(bufferUsage, size),
              flags(GL_MAP_WRITE_BIT) {
        glCreateBuffers(1, &buffer);
        switch (bufferUsage) {
//            case AllocationUsage::GPU_ONLY:
//            case AllocationUsage::CPU_TO_GPU:
//                break;
//            case AllocationUsage::CPU_ONLY:
//            case AllocationUsage::GPU_TO_CPU:
//                flags |= GL_CLIENT_STORAGE_BIT;
//                break;
            case Usage::VERTEX:
                break;
            case Usage::INDEX:
                break;
            case Usage::UNIFORM:
                break;
            case Usage::TRANSFER_DST:
                break;
            case Usage::TRANSFER_SRC:
                break;
        }

        glNamedBufferStorage(
                buffer,
                static_cast<GLsizeiptr>(size),
                nullptr,
                this->flags
        );
        spdlog::trace("Created new GL commandBuffer {} ({}B) and flags {}", buffer, size, this->flags);
    }

    GlBuffer::~GlBuffer() {
        glDeleteBuffers(1, &buffer);
    }

//    void *GlBuffer::map(std::size_t offset, std::size_t length) const {
//        if (offset > size)
//            error("Offset is greater than the total commandBuffer size");
//        if (offset + length > size)
//            error("The offset plus length is greater than the total commandBuffer size");
//
//        void *d = glMapNamedBufferRange(buffer, static_cast<GLsizeiptr>(offset), static_cast<GLsizeiptr>(length),
//                                        flags);
//        if (d == nullptr)
//            error("Failed to map GL commandBuffer {}", buffer);
//
//        spdlog::trace("Mapped GL commandBuffer {} into {} at offset {}B and length {}B using flags {}", buffer, d,
//                      offset,
//                      length, flags);
//        return d;
//    }

    void GlBuffer::write(const char *data, size_t dataSize, size_t offset) {
        if (offset + dataSize > size)
            throw std::runtime_error("Buffer overflow");

        char *d = map();
        memcpy(static_cast<char *>(d) + offset, data, dataSize);
        unmap();
    }

    char *GlBuffer::map() {
        return static_cast<char *>(glMapNamedBuffer(buffer, GL_WRITE_ONLY));
    }

    void GlBuffer::unmap() {
        glUnmapNamedBuffer(buffer);
    }

    GLuint GlBuffer::getBuffer() const {
        return buffer;
    }
}
