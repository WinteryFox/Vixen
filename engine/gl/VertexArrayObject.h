#pragma once

#include <GL/glew.h>
#include <unordered_map>
#include <vector>
#include "Buffer.h"

namespace Vixen::Engine::Gl {
    struct VertexBinding {
        struct Location {
            Location(GLuint index, GLint size, GLenum type, GLintptr offset, GLsizei stride)
                    : index(index), size(size), type(type), offset(offset), stride(stride) {}

            GLuint index;

            GLint size;

            GLenum type;

            GLintptr offset;

            GLsizei stride;
        };

        VertexBinding(const std::shared_ptr<Buffer> &buffer, const std::vector<Location> &locations)
                : buffer(buffer), locations(locations) {}

        std::shared_ptr<Buffer> buffer;

        std::vector<Location> locations;
    };

    class VertexArrayObject {
        GLuint vao{};

        std::unordered_map<GLuint, VertexBinding> bindings;

    public:
        explicit VertexArrayObject(const std::vector<VertexBinding> &bindings);

        VertexArrayObject(const VertexArrayObject &) = delete;

        VertexArrayObject &operator=(const VertexArrayObject &) = delete;

        ~VertexArrayObject();

        void bind() const;
    };
}
