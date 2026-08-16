#pragma once

#include <vector>

#include "RenderPass.h"
#include "RenderPassType.h"

namespace Vixen {
    class FrameGraph final {
        std::vector<ResourceNode> resources;

        std::vector<RenderPass> renderPasses;

        explicit FrameGraph(std::vector<RenderPass>&& renderPasses);

    public:
        FrameGraph(const FrameGraph& other) = delete;

        FrameGraph(FrameGraph&& other) noexcept = default;

        FrameGraph& operator=(const FrameGraph& other) = delete;

        FrameGraph& operator=(FrameGraph&& other) noexcept = default;

        ~FrameGraph() = default;

        class Builder {
            std::vector<ResourceNode> resources;

            std::vector<RenderPass> renderPasses;

        public:
            Builder() = default;

            Builder(const Builder& other) = delete;

            Builder(Builder&& other) noexcept = default;

            Builder& operator=(const Builder& other) = delete;

            Builder& operator=(Builder&& other) noexcept = default;

            ~Builder() = default;

            template <typename PassData, typename Setup, typename Execute>
            Builder& addGraphicsPass(
                std::string name,
                Setup&& setup,
                Execute&& execute
            ) {
                PassData data{};

                RenderPass::Builder passBuilder{
                    resources,
                    std::move(name),
                    RenderPassType::Graphics
                };

                std::invoke(std::forward<Setup>(setup), passBuilder, data);

                renderPasses.emplace_back(
                    std::move(passBuilder).build<PassData>(
                        std::move(data),
                        std::forward<Execute>(execute)
                    )
                );

                return *this;
            }

            [[nodiscard]] FrameGraph build() && {
                return FrameGraph{std::move(renderPasses)};
            }
        };
    };
} // namespace Vixen
