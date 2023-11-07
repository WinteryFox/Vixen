#pragma once

#include <unordered_map>
#include <vector>
#include "GlBuffer.h"

namespace Vixen::Gl {
    struct VertexBinding {
        struct Location {
            Location(GLuint index, GLint size, GLenum type, GLboolean normalized, GLintptr offset, GLsizei stride)
                    : index(index), size(size), type(type), normalized(normalized), offset(offset), stride(stride) {}

            GLuint index;

            GLint size;

            GLenum type;

            GLboolean normalized;

            GLintptr offset;

            GLsizei stride;
        };

        VertexBinding(uint32_t index, const std::shared_ptr<GlBuffer> &buffer, const std::vector<Location> &locations)
                : index(index), buffer(buffer), locations(locations) {}

        uint32_t index;

        std::shared_ptr<GlBuffer> buffer;

        std::vector<Location> locations;
    };

    class GlVertexArrayObject {
        GLuint vao{};

        std::unordered_map<GLuint, VertexBinding> bindings;

    public:
        std::size_t indexOffset;

        GlVertexArrayObject(const std::vector<VertexBinding> &bindings, std::size_t indexOffset);

        GlVertexArrayObject(const GlVertexArrayObject &) = delete;

        GlVertexArrayObject &operator=(const GlVertexArrayObject &) = delete;

        ~GlVertexArrayObject();

        void bind() const;

        void unbind() const;
    };
}
