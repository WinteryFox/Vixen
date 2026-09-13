#include "DescriptorPool.h"

namespace Vixen {
    DescriptorPool::DescriptorPool()
        : lifetime(std::make_shared<DescriptorPoolLifetime>()) {
        lifetime->owner = this;
    }

    DescriptorPool::~DescriptorPool() {
        lifetime->owner = nullptr;
    }
}
