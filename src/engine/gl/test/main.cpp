#include <fstream>
#include "../GlWindow.h"
#include "../GlShaderModule.h"
#include "../GlShaderProgram.h"
#include "GlBuffer.h"
#include "../GlVertexArrayObject.h"
#include "GlVixen.h"
#include <unistd.h>

#ifdef _WIN32

#include <windows.h>

#endif

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
};

int main() {
#ifdef _WIN32
    system(("chcp " + std::to_string(CP_UTF8)).c_str());
#endif
    spdlog::set_level(spdlog::level::trace);

    auto vixen = Vixen::Gl::GlVixen("Vixen OpenGL Test", {1, 0, 0});

    std::ifstream vertexStream("../../src/editor/shaders/triangle.vert");
    std::string vertexSource((std::istreambuf_iterator<char>(vertexStream)), std::istreambuf_iterator<char>());
    auto vertexModule = std::make_shared<Vixen::Gl::GlShaderModule>(Vixen::ShaderModule::Stage::VERTEX, vertexSource);

    std::ifstream fragmentStream("../../src/editor/shaders/triangle.frag");
    std::string fragmentSource((std::istreambuf_iterator<char>(fragmentStream)), std::istreambuf_iterator<char>());
    auto fragmentModule = std::make_shared<Vixen::Gl::GlShaderModule>(Vixen::ShaderModule::Stage::FRAGMENT,
                                                                      fragmentSource);

    Vixen::Gl::GlShaderProgram program(vertexModule, fragmentModule);

    std::vector<Vertex> vertices{
            {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f,  -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
            {{0.5f,  0.5f,  0.0f}, {0.0f, 0.0f, 1.0f}},
            {{-0.5f, 0.5f,  0.0f}, {1.0f, 1.0f, 1.0f}}
    };

    std::vector<uint32_t> indices{
            0, 1, 2,
            2, 3, 0
    };

    auto vbo = std::make_shared<Vixen::Gl::GlBuffer>(
            Vixen::Buffer::Usage::VERTEX | Vixen::Buffer::Usage::INDEX,
            vertices.size() * sizeof(Vertex) +
            indices.size() * sizeof(uint32_t)
    );
    vbo->write(
            reinterpret_cast<const char *>(vertices.data()),
            sizeof(Vertex) * vertices.size(),
            0
    );
    vbo->write(
            reinterpret_cast<const char *>(indices.data()),
            sizeof(uint32_t) * indices.size(),
            sizeof(Vertex) * vertices.size()
    );

    auto vao = Vixen::Gl::GlVertexArrayObject(
            {
                    Vixen::Gl::VertexBinding(
                            0,
                            sizeof(Vertex),
                            vbo,
                            {
                                    Vixen::Gl::VertexBinding::Location(
                                            0,
                                            3,
                                            GL_FLOAT,
                                            GL_FALSE,
                                            offsetof(Vertex, position)
                                    ),
                                    Vixen::Gl::VertexBinding::Location(
                                            1,
                                            3,
                                            GL_FLOAT,
                                            GL_FALSE,
                                            offsetof(Vertex, color)
                                    )
                            }
                    )
            },
            vertices.size() * sizeof(Vertex)
    );

    double old = glfwGetTime();
    uint32_t fps;
    while (!vixen.window.shouldClose()) {
        vixen.window.clear();

        program.bind();
        vao.bind();
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, (void *) vao.indexOffset);

        vixen.window.update();
        vixen.window.swap();

        fps++;
        double now = glfwGetTime();
        if (now - old >= 1) {
            spdlog::info("FPS: {}", fps);
            old = now;
            fps = 0;
        }
    }
    return EXIT_SUCCESS;
}
