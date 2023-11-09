#include "GlRenderer.h"

namespace Vixen::Gl {
    GlRenderer::GlRenderer() {

    }

    void GlRenderer::submit() {
        /*for (const auto &pass: passes) {
            for (const auto &material: pass.materials) {
                material.shader.bind();

                for (const auto &texture: material.textures) {
                    std::vector<DrawElementsIndirectCommand> commands;
                    texture.bind();

                    for (const auto &object : texture.entities) {
                        commands.emplace_back(DrawElementsIndirectCommand{
                                .count = object.indexCount,
                                // TODO: Group instances
                                .instanceCount = 1,
                                .firstIndex = 0,
                                .baseVertex = 0,
                                .baseInstance = 0,
                        });
                    }

                    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, commands.data(),
                                                static_cast<GLsizei>(commands.size()), 0);
                }
            }
        }*/
    }
}
