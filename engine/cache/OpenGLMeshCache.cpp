#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include "OpenGLMeshCache.h"

namespace Vixen::Engine::Cache {
    OpenGLMeshCache::OpenGLMeshCache() : MeshCache(
            static_cast<aiPostProcessSteps>(aiProcess_CalcTangentSpace | aiProcess_Triangulate |
                                            aiProcess_JoinIdenticalVertices | aiProcess_SortByPType)
    ) {}

    // TODO: We can't just return this, needs to be our shared_ptr
    Gl::Buffer
    OpenGLMeshCache::load(const std::string &path, AllocationUsage allocationUsage, BufferUsage bufferUsage) {
        const aiScene *scene = loadFile(path);

        if (!scene) {
            std::string error = "Failed to load mesh file " + path + "\n    ";

            throw std::runtime_error(error.append(importer.GetErrorString()));
        }

        std::size_t bufferSize = 0;

        for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex += 1) {
            const auto &mesh = scene->mMeshes[meshIndex];

            std::vector<glm::vec3> vertices;
            std::vector<uint32_t> indices;
            std::vector<glm::vec2> uvs;
            std::vector<glm::vec4> colors;

            vertices.reserve(mesh->mNumVertices);
            indices.reserve(mesh->mNumFaces * 3);

            for (unsigned int vertexIndex = 0; vertexIndex < mesh->mNumVertices; vertexIndex += 1) {
                const auto &vertex = mesh->mVertices[vertexIndex];

                vertices.emplace_back(vertex.x, vertex.y, vertex.z);

                if (mesh->HasTextureCoords(0)) {
                    const auto &uv = mesh->mTextureCoords[0][vertexIndex];

                    uvs.emplace_back(uv.x, uv.y);
                } else {
                    uvs.emplace_back(0.0f, 0.0f);
                }

                if (mesh->HasVertexColors(0)) {
                    const auto &color = mesh->mColors[0][vertexIndex];

                    colors.emplace_back(color.r, color.g, color.b, color.a);
                } else {
                    colors.emplace_back(1.0f, 1.0f, 1.0f, 1.0f);
                }
            }

            for (unsigned int faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++) {
                const auto &face = mesh->mFaces[faceIndex];

                if (face.mNumIndices != 3) {
                    throw std::runtime_error("Mesh " + path + " contains a non-triangulated face.");
                }

                indices.push_back(face.mIndices[0]);
                indices.push_back(face.mIndices[1]);
                indices.push_back(face.mIndices[2]);
            }

            // TODO: Textures; ref: https://github.com/WinteryFox/VixenEngineVulkan/blob/master/engine/src/MeshStore.h
        }

        // TODO: This is just here to make it compile, we need to rethink this whole approach a bit
        return { bufferSize, bufferUsage, allocationUsage };
    }
}