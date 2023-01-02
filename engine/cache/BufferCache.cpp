#include "BufferCache.h"

namespace Vixen::Engine::Cache {
    template<class T>
    void BufferCache<T>::clear() {
        cache.clear();
    }

    template<class T>
    bool BufferCache<T>::contains(const std::string &path) {
        return cache.contains(path);
    }

    template<class T>
    std::shared_ptr<T>
    BufferCache<T>::get(const std::string &path, AllocationUsage allocationUsage, BufferUsage bufferUsage) {
        if (!contains(path)) {
            T loaded = load(path, allocationUsage, bufferUsage);

            cache[path] = std::make_shared<T>(loaded);
        }

        return cache[path];
    }

    template<class T>
    bool BufferCache<T>::remove(const std::string &path) {
        return cache.erase(path) == 1;
    }
}
