#include <fstream>
#include "gl/Window.h"
#include "gl/ShaderModule.h"
#include "gl/ShaderProgram.h"
#include "gl/Buffer.h"
#include <windows.h>
#include <unistd.h>

using namespace Vixen::Engine;

int main() {
    system(("chcp " + std::to_string(CP_UTF8)).c_str());
    spdlog::set_level(spdlog::level::trace);

    std::vector<float> vertices = {
            -0.5f, -0.5f, 0.0f,
            0.5f, -0.5f, 0.0f,
            0.0f, 0.5f, 0.0f
    };

    auto window = Vixen::Engine::Gl::Window("Vixen Editor", 720, 480, true);
    window.center();
    window.setClearColor(0.13f, 0.23f, 0.33f, 1.0f);
    window.setVisible(true);

    std::ifstream vertexStream("../../editor/shaders/triangle.vert");
    std::string vertexSource((std::istreambuf_iterator<char>(vertexStream)), std::istreambuf_iterator<char>());
    auto vertexModule = std::make_shared<Gl::ShaderModule>(Vixen::Engine::ShaderModule::Stage::VERTEX, vertexSource);

    std::ifstream fragmentStream("../../editor/shaders/triangle.frag");
    std::string fragmentSource((std::istreambuf_iterator<char>(fragmentStream)), std::istreambuf_iterator<char>());
    auto fragmentModule = std::make_shared<Gl::ShaderModule>(Vixen::Engine::ShaderModule::Stage::FRAGMENT,
                                                             fragmentSource);

    Gl::ShaderProgram program({vertexModule, fragmentModule});

    unsigned int vao;
    glCreateVertexArrays(1, &vao);

    auto vbo = Vixen::Engine::Gl::Buffer<float>(vertices.size(), BufferUsage::VERTEX, AllocationUsage::GPU_ONLY);
    vbo.write(vertices, 0);

    //glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glVertexArrayVertexBuffer(vao, 0, vbo.buffer, 0, 3 * sizeof(float));
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribBinding(vao, 0, 0);

    while (!window.shouldClose()) {
        window.clear();

        program.bind();
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        Vixen::Engine::Gl::Window::update();
        window.swap();
    }
    exit(EXIT_SUCCESS);
}
