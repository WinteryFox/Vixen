#include "GlVertexArrayObject.h"

namespace Vixen::Gl {
    GlVertexArrayObject::GlVertexArrayObject(const std::vector<VertexBinding> &bindings, size_t indexOffset)
            : bindings(), indexOffset(indexOffset) {
        glCreateVertexArrays(1, &vao);

        for (const auto &binding: bindings) {
            for (const auto &location: binding.locations) {
                glVertexArrayVertexBuffer(vao, location.index, binding.buffer->getBuffer(), location.offset,
                                          static_cast<GLsizei>(binding.stride));
                glVertexArrayAttribFormat(vao, location.index, location.size, location.type, location.normalized,
                                          location.offset);
                glEnableVertexArrayAttrib(vao, location.index);
                glVertexArrayAttribBinding(vao, location.index, binding.index);
            }

            if (binding.buffer->getBufferUsage() & Buffer::Usage::INDEX)
                glVertexArrayElementBuffer(vao, binding.buffer->getBuffer());
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
