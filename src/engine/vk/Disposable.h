#pragma once

namespace Vixen::Vk {
    class Disposable {
    public:
        virtual ~Disposable() = default;

        virtual void dispose() const = 0;
    };
}
