#include "GlVertexArrayObject.h"

namespace Vixen::Engine {
    GlVertexArrayObject::GlVertexArrayObject(const std::vector<VertexBinding> &bindings, size_t indexOffset)
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

    GlVertexArrayObject::~GlVertexArrayObject() {
        glDeleteVertexArrays(1, &vao);
    }

    void GlVertexArrayObject::bind() const {
        glBindVertexArray(vao);
    }

    void GlVertexArrayObject::unbind() const {
        glBindVertexArray(0);
    }
}
