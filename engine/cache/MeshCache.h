#pragma once

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include "BufferCache.h"
#include <string>

namespace Vixen::Engine::Cache {
    template<class T = Buffer>
    class MeshCache : BufferCache<T> {
    private:
        aiPostProcessSteps postProcessSteps;

    public:
        explicit MeshCache(aiPostProcessSteps postProcessSteps);

    protected:
        const aiScene *loadFile(const std::string &path);

        Assimp::Importer importer = {};
    };
}
