#include "../Buffer.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace Vixen::Engine::Cache {
    template<class T = Buffer>
    class BufferCache {
    private:
        std::unordered_map<std::string, std::shared_ptr<T>> cache = {};

    protected:
        virtual T load(const std::string &path, AllocationUsage allocationUsage, BufferUsage bufferUsage) = 0;

    public:
        void clear();

        bool contains(const std::string &path);

        std::shared_ptr<T> get(const std::string &path, AllocationUsage allocationUsage, BufferUsage bufferUsage);

        bool remove(const std::string &path);
    };
}
