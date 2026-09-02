#pragma once

#include "PipelineLayout.h"

namespace Vixen {
    class Pipeline {
        const PipelineLayout& layout;

    protected:
        explicit Pipeline(const PipelineLayout& layout);

    public:
        Pipeline(const Pipeline& other) = delete;
        Pipeline& operator=(const Pipeline& other) = delete;

        Pipeline(Pipeline&& other) noexcept = delete;
        Pipeline& operator=(Pipeline&& other) noexcept = delete;

        virtual ~Pipeline() = default;

        [[nodiscard]]
        const PipelineLayout& getLayout() const noexcept;
    };
}
