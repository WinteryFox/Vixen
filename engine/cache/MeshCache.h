#include <scene.h>
#include <Importer.hpp>
#include <postprocess.h>
#include "BufferCache.h"
#include "string"

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
