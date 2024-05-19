#pragma once

namespace Vixen {
    class Disposable {
    public:
        virtual ~Disposable() = default;

        virtual void dispose() const = 0;
    };
}
