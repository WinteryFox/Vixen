#pragma once

#include <unordered_map>
#include <vector>
#include "GlBuffer.h"

namespace Vixen::Gl {
    struct VertexBinding {
        struct Location {
            Location(GLuint index, GLint size, GLenum type, GLboolean normalized, GLintptr offset)
                    : index(index), size(size), type(type), normalized(normalized), offset(offset) {}

            GLuint index;

            GLint size;

            GLenum type;

            GLboolean normalized;

            GLintptr offset;
        };

        VertexBinding(uint32_t index, uint32_t stride, const std::shared_ptr<GlBuffer> &buffer,
                      const std::vector<Location> &locations)
                : index(index), stride(stride), buffer(buffer), locations(locations) {}

        uint32_t index;

        uint32_t stride;

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
