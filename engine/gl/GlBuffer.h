#pragma once

#include <GL/glew.h>
#include <spdlog/spdlog.h>
#include "../Buffer.h"

namespace Vixen::Engine {
    class GlBuffer : public Buffer {
    public:
        using Buffer::write;
        GLuint buffer{};

        GlBuffer(const size_t &size, BufferUsage bufferUsage, AllocationUsage allocationUsage)
                : Engine::Buffer(size, bufferUsage, allocationUsage) {
            glCreateBuffers(1, &buffer);

            GLenum u; // TODO: It's likely this will need tweaks later on
            if (bufferUsage & BufferUsage::VERTEX) {
                u = GL_STATIC_DRAW;
            } else if (bufferUsage & BufferUsage::INDEX) {
                u = GL_STATIC_DRAW;
            } else if (bufferUsage & BufferUsage::UNIFORM) {
                u = GL_DYNAMIC_DRAW;
            } else if (bufferUsage & BufferUsage::TRANSFER_DST) {
                switch (allocationUsage) {
                    case AllocationUsage::GPU_ONLY:
                        u = GL_STREAM_COPY;
                        break;
                    case AllocationUsage::CPU_ONLY:
                    case AllocationUsage::CPU_TO_GPU:
                        u = GL_STREAM_DRAW;
                        break;
                    case AllocationUsage::GPU_TO_CPU:
                        u = GL_STREAM_READ;
                        break;
                }
            } else if (bufferUsage & BufferUsage::TRANSFER_SRC) {
                switch (allocationUsage) {
                    case AllocationUsage::GPU_ONLY:
                        u = GL_STREAM_COPY;
                        break;
                    case AllocationUsage::CPU_ONLY:
                    case AllocationUsage::CPU_TO_GPU:
                        u = GL_STREAM_READ;
                        break;
                    case AllocationUsage::GPU_TO_CPU:
                        u = GL_STREAM_DRAW;
                        break;
                }
            }

            glNamedBufferData(buffer, static_cast<GLsizeiptr>(size), nullptr, u);
        }

        Buffer &write(const void *data, std::size_t dataSize, std::size_t offset) override {
            glNamedBufferSubData(buffer, static_cast<GLsizeiptr>(offset), static_cast<GLsizeiptr>(dataSize), data);
            return *this;
        }

        void *map() override {
            return (void *) glMapNamedBuffer(buffer, GL_READ_WRITE);
        }

        void unmap() override {
            if (!glUnmapNamedBuffer(buffer))
                spdlog::error("Failed to unmap GL buffer");
        }
    };
}
