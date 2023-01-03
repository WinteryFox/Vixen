#include "MeshCache.h"

namespace Vixen::Engine::Cache {
    template<class T>
    MeshCache<T>::MeshCache(aiPostProcessSteps postProcessSteps) : postProcessSteps(postProcessSteps) {}

    template<class T>
    const aiScene *MeshCache<T>::loadFile(const std::string &path) {
        return importer.ReadFile(path, postProcessSteps);
    }
}
