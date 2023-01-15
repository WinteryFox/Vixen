#include <fstream>
#include "gl/GlWindow.h"
#include "gl/GlShaderModule.h"
#include "gl/GlShaderProgram.h"
#include "gl/GlBuffer.h"
#include "gl/GlVertexArrayObject.h"
#include <unistd.h>

#ifdef _WIN32

#include <windows.h>

#endif

using namespace Vixen::Engine;

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

    auto window = GlWindow("Vixen OpenGL Test", 720, 480, true);
    window.center();
    window.setClearColor(0.13f, 0.23f, 0.33f, 1.0f);
    window.setVisible(true);

    std::ifstream vertexStream("../../editor/shaders/triangle.vert");
    std::string vertexSource((std::istreambuf_iterator<char>(vertexStream)), std::istreambuf_iterator<char>());
    auto vertexModule = std::make_shared<GlShaderModule>(Vixen::Engine::ShaderModule::Stage::VERTEX, vertexSource);

    std::ifstream fragmentStream("../../editor/shaders/triangle.frag");
    std::string fragmentSource((std::istreambuf_iterator<char>(fragmentStream)), std::istreambuf_iterator<char>());
    auto fragmentModule = std::make_shared<GlShaderModule>(Vixen::Engine::ShaderModule::Stage::FRAGMENT,
                                                           fragmentSource);

    GlShaderProgram program({vertexModule, fragmentModule});

    auto vbo = std::make_shared<GlBuffer>(
            vertices.size() * sizeof(glm::vec3) + indices.size() * sizeof(std::uint32_t),
            Vixen::Engine::BufferUsage::VERTEX | Vixen::Engine::BufferUsage::INDEX,
            Vixen::Engine::AllocationUsage::GPU_ONLY
    );
    vbo->write(vertices, 0);
    vbo->write(indices, vertices.size() * sizeof(glm::vec3));

    auto vao = GlVertexArrayObject(
            {
                    VertexBinding(
                            vbo,
                            {
                                    VertexBinding::Location(0, 3, GL_FLOAT, GL_FALSE, 0, sizeof(glm::vec3))
                            }
                    )
            },
            vertices.size() * sizeof(glm::vec3)
    );

    while (!window.shouldClose()) {
        window.clear();

        program.bind();
        vao.bind();
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, (void*) vao.indexOffset);

        GlWindow::update();
        window.swap();
    }
    return EXIT_SUCCESS;
}
