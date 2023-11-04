#include <fstream>
#include "../GlWindow.h"
#include "../GlShaderModule.h"
#include "../GlShaderProgram.h"
#include "GlBuffer.h"
#include "../GlVertexArrayObject.h"
#include <unistd.h>

#ifdef _WIN32

#include <windows.h>

#endif

int main() {
#ifdef _WIN32
    system(("chcp " + std::to_string(CP_UTF8)).c_str());
#endif
    spdlog::set_level(spdlog::level::trace);

    std::vector<glm::vec3> vertices = {
            {0.5f,  0.5f,  0.0f},  // top right
            {0.5f,  -0.5f, 0.0f},  // bottom right
            {-0.5f, -0.5f, 0.0f},  // bottom left
            {-0.5f, 0.5f,  0.0f}   // top left
    };
    std::vector<std::uint32_t> indices = {  // note that we start from 0!
            0, 1, 3,   // first triangle
            1, 2, 3    // second triangle
    };

    auto window = Vixen::Gl::GlWindow("Vixen OpenGL Test", 720, 480, true);
    window.center();
    window.setVisible(true);

    std::ifstream vertexStream("../../editor/shaders/triangle.vert");
    std::string vertexSource((std::istreambuf_iterator<char>(vertexStream)), std::istreambuf_iterator<char>());
    auto vertexModule = std::make_shared<Vixen::Gl::GlShaderModule>(Vixen::ShaderModule::Stage::VERTEX, vertexSource);

    std::ifstream fragmentStream("../../editor/shaders/triangle.frag");
    std::string fragmentSource((std::istreambuf_iterator<char>(fragmentStream)), std::istreambuf_iterator<char>());
    auto fragmentModule = std::make_shared<Vixen::Gl::GlShaderModule>(Vixen::ShaderModule::Stage::FRAGMENT,
                                                                      fragmentSource);

    Vixen::Gl::GlShaderProgram program(vertexModule, fragmentModule);

    auto vbo = std::make_shared<Vixen::Gl::GlBuffer>(
            Vixen::Buffer::Usage::VERTEX | Vixen::Buffer::Usage::INDEX,
            vertices.size() * sizeof(glm::vec3) +
            indices.size() * sizeof(std::uint32_t)
    );
    vbo->write(reinterpret_cast<const char *>(vertices.data()), vertices.size() * sizeof(glm::vec3), 0);
    vbo->write(reinterpret_cast<const char *>(indices.data()), vertices.size() * sizeof(glm::vec3), 0);

    auto vao = Vixen::Gl::GlVertexArrayObject(
            {
                    Vixen::Gl::VertexBinding(
                            vbo,
                            {
                                    Vixen::Gl::VertexBinding::Location(0, 3, GL_FLOAT, GL_FALSE, 0, sizeof(glm::vec3))
                            }
                    )
            },
            vertices.size() * sizeof(glm::vec3)
    );

    while (!window.shouldClose()) {
        window.clear();

        program.bind();
        vao.bind();
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, (void *) vao.indexOffset);

        window.update();
        window.swap();
    }
    return EXIT_SUCCESS;
}
