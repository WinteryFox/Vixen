#pragma once

#include <deque>

#include "Disposable.h"

namespace Vixen {
    class DeletionQueue {
        std::deque<Disposable> queue;

    public:
        DeletionQueue();

        DeletionQueue(const DeletionQueue &) = delete;

        DeletionQueue &operator=(const DeletionQueue &) = delete;

        DeletionQueue(DeletionQueue &&other) noexcept;

        DeletionQueue &operator=(DeletionQueue &&other) noexcept;

        void flush();
    };
}
