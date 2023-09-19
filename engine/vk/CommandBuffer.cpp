#include "CommandBuffer.h"

namespace Vixen::Vk {
    CommandBuffer::CommandBuffer(const std::shared_ptr<Vk::Device> &device)
            : device(device) {

    }

    CommandBuffer::~CommandBuffer() {

    }
}
