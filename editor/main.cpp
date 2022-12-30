#include <fstream>
#include "gl/Window.h"
#include "gl/ShaderModule.h"
#include "gl/ShaderProgram.h"
#include <windows.h>
#include <unistd.h>

using namespace Vixen::Engine;

int main() {
    system(("chcp " + std::to_string(CP_UTF8)).c_str());
    spdlog::set_level(spdlog::level::trace);

    float vertices[] = {
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
    glGenVertexArrays(1, &vao);
    unsigned int vbo;
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

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
