#include "DeletionQueue.h"

#include <spdlog/spdlog.h>

namespace Vixen {
    DeletionQueue::DeletionQueue()
        : queue(std::deque<Disposable>()) {}

    DeletionQueue::DeletionQueue(DeletionQueue &&other) noexcept
        : queue(std::move(other.queue)) {}

    DeletionQueue &DeletionQueue::operator=(DeletionQueue &&other) noexcept {
        std::swap(queue, other.queue);

        return *this;
    }

    void DeletionQueue::flush() {
        spdlog::trace("Flushing deletion queue of {} items", queue.size());
        for (const auto &disposable: queue)
            disposable.dispose();

        queue.clear();
    }
}
