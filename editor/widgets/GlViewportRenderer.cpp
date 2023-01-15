#include "GlViewportRenderer.h"

bool glewInitialized = false;

namespace Vixen::Editor {
    void GlViewportRenderer::setViewportSize(const QSize &size) {
        viewportSize = size;
    }

    void GlViewportRenderer::setWindow(QQuickWindow *win) {
        window = win;
    }

    void GlViewportRenderer::init() {
        if (!glewInitialized) {
            glewInitialized = true;
            glewExperimental = GL_TRUE;
            auto result = glewInit();
            if (GLEW_OK != result) {
                spdlog::error("Failed to initialize glew: {}",
                              reinterpret_cast<const char *>(glewGetErrorString(result)));
                throw std::runtime_error("Failed to initialize glew");
            }

#ifdef DEBUG
            if (GLEW_ARB_debug_output) {
                spdlog::debug("Enabling OpenGL debug extension");
                glEnable(GL_DEBUG_OUTPUT);
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
                glDebugMessageCallback(glDebugCallback, nullptr);
                glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_ERROR, GL_DONT_CARE, 0, nullptr, GL_TRUE);
            }
#endif
        }

        if (!program) {
            std::ifstream vertexStream("../../editor/shaders/triangle.vert");
            std::string vertexSource((std::istreambuf_iterator<char>(vertexStream)), std::istreambuf_iterator<char>());
            auto vertexModule = std::make_shared<GlShaderModule>(Vixen::Engine::ShaderModule::Stage::VERTEX,
                                                                 vertexSource);

            std::ifstream fragmentStream("../../editor/shaders/triangle.frag");
            std::string fragmentSource((std::istreambuf_iterator<char>(fragmentStream)),
                                       std::istreambuf_iterator<char>());
            auto fragmentModule = std::make_shared<GlShaderModule>(Vixen::Engine::ShaderModule::Stage::FRAGMENT,
                                                                   fragmentSource);

            program = std::make_shared<GlShaderProgram>(
                    std::vector<std::shared_ptr<GlShaderModule>>{vertexModule, fragmentModule});
        }

        if (!vbo) {
            vbo = std::make_shared<GlBuffer>(
                    vertices.size() * sizeof(glm::vec3) + indices.size() * sizeof(std::uint32_t),
                    Vixen::Engine::BufferUsage::VERTEX | Vixen::Engine::BufferUsage::INDEX,
                    Vixen::Engine::AllocationUsage::GPU_ONLY
            );
            vbo->write(vertices, 0);
            vbo->write(indices, vertices.size() * sizeof(glm::vec3));
        }

        if (!vao) {
            vao = std::make_shared<GlVertexArrayObject>(
                    std::vector<VertexBinding>{
                            VertexBinding(
                                    vbo,
                                    {
                                            VertexBinding::Location(0, 3, GL_FLOAT, GL_FALSE, 0, sizeof(glm::vec3))
                                    }
                            )
                    },
                    vertices.size() * sizeof(glm::vec3)
            );
        }
    }

    void GlViewportRenderer::paint() {
        window->beginExternalCommands();

        glViewport(0, 0, viewportSize.width(), viewportSize.height());
        glDisable(GL_BLEND);

        program->bind();
        vao->bind();

        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, (void *) vao->indexOffset);

        glBindVertexArray(0);
        glUseProgram(0);

        window->endExternalCommands();
    }
}
