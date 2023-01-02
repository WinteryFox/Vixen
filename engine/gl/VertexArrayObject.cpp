#include "VertexArrayObject.h"

namespace Vixen::Engine::Gl {

    VertexArrayObject::VertexArrayObject(const std::vector<VertexBinding> &bindings, size_t indexOffset)
            : bindings(), indexOffset(indexOffset) {
        glCreateVertexArrays(1, &vao);

        for (const auto &binding: bindings) {
            for (const auto &location: binding.locations) {
                glVertexArrayVertexBuffer(vao, location.index, binding.buffer->buffer, location.offset,
                                          location.stride);
                glVertexArrayAttribFormat(vao, location.index, location.size, location.type, location.normalized,
                                          location.offset);
                glEnableVertexArrayAttrib(vao, location.index);
                // TODO: The binding index doesn't necessarily need to be the same as the attribute index
                glVertexArrayAttribBinding(vao, location.index, location.index);
            }

            if (binding.buffer->bufferUsage & BufferUsage::INDEX)
                glVertexArrayElementBuffer(vao, binding.buffer->buffer);
        }
    }

    VertexArrayObject::~VertexArrayObject() {
        glDeleteVertexArrays(1, &vao);
    }

    void VertexArrayObject::bind() const {
        glBindVertexArray(vao);
    }
}