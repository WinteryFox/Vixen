#pragma once

#include <GL/glew.h>
#include "../Buffer.h"

namespace Vixen::Engine::Gl {
    template<typename T>
    class Buffer : Engine::Buffer<T> {
        GLuint buffer{};

    public:
        /*explicit Buffer(const size_t size) : Buffer(size) {
            glGenBuffers(1, &buffer);
            switch (usage)
            glBufferData(buffer, size, nullptr, );
        }*/
        Buffer(const size_t size, BufferUsage usage, BufferType type) : Buffer(size, usage, type) {
            glGenBuffers(1, &buffer);

            GLenum t;
            switch (type) {
                case BufferType::TRANSFER_DST:
                    break;
                case BufferType::TRANSFER_SRC:
                    break;
                case BufferType::ARRAY:
                    t = GL_ARRAY_BUFFER;
                    break;
                case BufferType::UNIFORM:
                    break;
            }
            glBufferData(t, buffer, nullptr, GL_STATIC_DRAW);
        }
    };
}
